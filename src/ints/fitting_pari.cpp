#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/SVD>
#include "extern/lapack.hpp"
#include "ints/fitting.hpp"
#include "math/solvers/hermitian_eigen_solver.hpp"
#include "utils/constants.hpp"
#include "utils/scheduler.hpp"

namespace megalochem {

namespace ints {

dbcsr::sbtensor<3, double> dfitting::compute_pari(
    dbcsr::shared_matrix<double> s_xx,
    shared_screener scr_s,
    std::array<int, 3> bdims,
    dbcsr::btype mytype)
{
  auto aofac = std::make_shared<aofactory>(m_mol, m_world);
  aofac->ao_3c2e_setup(metric::coulomb);

  auto& time_setup = TIME.sub("Setting up preliminary data");

  TIME.start();

  time_setup.start();

  auto s_xx_desym = s_xx->desymmetrize();
  s_xx_desym->replicate_all();

  // ================== get mapping ==================================

  auto b = m_mol->dims().b();
  auto x = m_mol->dims().x();

  arrvec<int, 2> bb = {b, b};
  arrvec<int, 3> xbb = {x, b, b};
  arrvec<int, 2> xx = {x, x};

  auto cbas = m_mol->c_basis();
  auto xbas = m_mol->c_dfbasis();

  auto atoms = m_mol->atoms();
  int natoms = atoms.size();

  auto blkmap_b = cbas->block_to_atom(atoms);
  auto blkmap_x = xbas->block_to_atom(atoms);

  LOG.os<1>("Block to atom mapping (b): \n");
  if (LOG.global_plev() >= 1) {
    for (auto e : blkmap_b) { LOG.os<1>(e, " "); }
    LOG.os<1>('\n');
  }

  LOG.os<1>("Block to atom mapping (x): \n");
  if (LOG.global_plev() >= 1) {
    for (auto e : blkmap_x) { LOG.os<1>(e, " "); }
    LOG.os<1>('\n');
  }

  // === END ===

  // ==================== new atom centered dists ====================

  auto spgrid2 = dbcsr::pgrid<2>::create(m_cart.comm()).build();

  auto spgrid3_xbb = dbcsr::pgrid<3>::create(m_cart.comm()).build();

  auto spgrid2_self = dbcsr::pgrid<2>::create(MPI_COMM_SELF).build();

  auto spgrid3_self = dbcsr::pgrid<3>::create(MPI_COMM_SELF).build();

  LOG.os<>("Grid size: ", m_cart.nprow(), " ", m_cart.npcol(), '\n');

  // === END

  // ===================== setup tensors =============================

  arrvec<int, 3> blkmaps = {blkmap_x, blkmap_b, blkmap_b};

  auto c_xbb_batched = dbcsr::btensor<3>::create()
                           .name("cpari_xbb_batched")
                           .set_pgrid(spgrid3_xbb)
                           .blk_sizes(xbb)
                           .blk_maps(blkmaps)
                           .batch_dims(bdims)
                           .btensor_type(mytype)
                           .print(1)
                           .build();

  auto c_xbb_global = dbcsr::tensor<3>::create()
                          .name("fitting coefficients")
                          .set_pgrid(*spgrid3_xbb)
                          .blk_sizes(xbb)
                          .map1({0})
                          .map2({1, 2})
                          .build();

  auto c_xbb_local = dbcsr::tensor<3>::create()
                         .name("c_xbb_local")
                         .set_pgrid(*spgrid3_self)
                         .map1({0})
                         .map2({1, 2})
                         .blk_sizes(xbb)
                         .build();

  auto c_xbb_AB = dbcsr::tensor<3>::create()
                      .name("c_xbb_AB")
                      .set_pgrid(*spgrid3_self)
                      .map1({0})
                      .map2({1, 2})
                      .blk_sizes(xbb)
                      .build();

  auto eri_local =
      dbcsr::tensor<3>::create_template(*c_xbb_local).name("eri_local").build();

  auto inv_local = dbcsr::tensor<2>::create()
                       .name("inv_local")
                       .set_pgrid(*spgrid2_self)
                       .map1({0})
                       .map2({1})
                       .blk_sizes(xx)
                       .build();

  arrvec<int, 3> blkidx = c_xbb_local->blks_local();
  arrvec<int, 3> blkoffsets = c_xbb_local->blk_offsets();

  c_xbb_batched->compress_init({2}, vec<int>{0}, vec<int>{1, 2});

  int nbatches = c_xbb_batched->nbatches(2);

  // Loop over batches N - they are guaranteed to always include all
  // shells of an atom, i.e. blocks belonging to the same atom are not
  // separated

  int64_t task_off = 0;

  for (int inu = 0; inu != nbatches; ++inu) {
    auto nu_bds = c_xbb_batched->blk_bounds(2, inu);

    // get atom range
    int atom_lb = natoms - 1;
    int atom_ub = 0;

    for (int iblk = nu_bds[0]; iblk != nu_bds[1] + 1; ++iblk) {
      atom_lb = std::min(atom_lb, blkmap_b[iblk]);
      atom_ub = std::max(atom_ub, blkmap_b[iblk]);
    }

    LOG.os<1>("BATCH: ", inu, " with atoms [", atom_lb, ",", atom_ub, "]\n");

    int64_t ntasks = (atom_ub - atom_lb + 1) * natoms;
    int rank = m_cart.rank();

    std::function<void(int64_t)> workfunc = [rank, natoms, &blkmap_b, &blkmap_x,
                                             &aofac, &inv_local, &eri_local,
                                             &c_xbb_AB, &c_xbb_local,
                                             &s_xx_desym, &scr_s, &x, &b,
                                             &task_off](int64_t itask) {
      int atomA = (itask + task_off) % (int64_t)natoms;
      int atomB = (itask + task_off) / (int64_t)natoms;

      std::cout << rank << ": ATOMS " << atomA << " " << atomB << std::endl;

      // Get mu/nu indices centered on alpha/beta
      vec<int> mA_idx, nB_idx, xAB_idx;

      for (size_t m = 0; m != b.size(); ++m) {
        if (blkmap_b[m] == atomA)
          mA_idx.push_back(int(m));
        if (blkmap_b[m] == atomB)
          nB_idx.push_back(int(m));
      }

      // get x indices centered on alpha or beta

      for (size_t i = 0; i != x.size(); ++i) {
        if (blkmap_x[i] == atomA || blkmap_x[i] == atomB)
          xAB_idx.push_back(int(i));
      }

#if 1
      std::cout << "INDICES: " << std::endl;

      for (auto m : mA_idx) { std::cout << m << " "; }
      std::cout << std::endl;

      for (auto n : nB_idx) { std::cout << n << " "; }
      std::cout << std::endl;

      for (auto ix : xAB_idx) { std::cout << ix << " "; }
      std::cout << std::endl;
#endif

      // compute 3c2e ints

      arrvec<int, 3> blks = {xAB_idx, mA_idx, nB_idx};

      aofac->ao_3c_fill_idx(eri_local, blks, scr_s);
      eri_local->filter(dbcsr::global::filter_eps);

      if (eri_local->num_blocks_total() == 0)
        return;

      int m = 0;

      // make mapping for compressed metric matrix
      /* | x 0 0 |      | x 0 |
       * | 0 y 0 |  ->  | 0 y |
       * | 0 0 z |
       */

      vec<int> xAB_sizes(xAB_idx.size());
      vec<int> xAB_offs(xAB_idx.size());
      std::map<int, int> xAB_mapping;

      int off = 0;
      for (size_t k = 0; k != xAB_idx.size(); ++k) {
        int kAB = xAB_idx[k];
        m += x[kAB];
        xAB_sizes[k] = x[kAB];
        xAB_offs[k] = off;
        xAB_mapping[kAB] = int(k);
        off += xAB_sizes[k];
      }

      // get (alpha|beta) matrix
      Eigen::MatrixXd alphaBeta = Eigen::MatrixXd::Zero(m, m);

#if 0
			auto met = dbcsr::tensor<2>::create_template(**inv_local)
				.name("metric").build();
				
			met->reserve_all();
#endif
      // loop over xab blocks
      for (auto ix : xAB_idx) {
        for (auto iy : xAB_idx) {
          int xsize = x[ix];
          int ysize = x[iy];

          bool found = false;
          double* ptr = s_xx_desym->get_block_data(ix, iy, found);

          if (found) {
#if 0		
						std::array<int,2> idx = {ix,iy};
						met->put_block(idx, ptr, sizes);
#endif
            Eigen::Map<Eigen::MatrixXd> blk(ptr, xsize, ysize);

            int xoff = xAB_offs[xAB_mapping[ix]];
            int yoff = xAB_offs[xAB_mapping[iy]];

            alphaBeta.block(xoff, yoff, xsize, ysize) = blk;
          }
        }
      }

#if 0
			std::cout << "Problem size: " << m << " x " << m << std::endl;
#endif

      Eigen::MatrixXd U = Eigen::MatrixXd::Zero(m, m);
      Eigen::MatrixXd Vt = Eigen::MatrixXd::Zero(m, m);
      Eigen::VectorXd s = Eigen::VectorXd::Zero(m);

      int info = 1;
      double worksize = 0;

      Eigen::MatrixXd alphaBeta_c = alphaBeta;

      c_dgesvd(
          'A', 'A', m, m, alphaBeta.data(), m, s.data(), U.data(), m, Vt.data(),
          m, &worksize, -1, &info);

      int lwork = (int)worksize;
      double* work = new double[lwork];

      c_dgesvd(
          'A', 'A', m, m, alphaBeta.data(), m, s.data(), U.data(), m, Vt.data(),
          m, work, lwork, &info);

      for (int i = 0; i != m; ++i) { s(i) = 1.0 / s(i); }

      Eigen::MatrixXd Inv = Eigen::MatrixXd::Zero(m, m);
      Inv = Vt.transpose() * s.asDiagonal() * U.transpose();

#if 0
			Eigen::MatrixXd E = Eigen::MatrixXd::Identity(m,m);
			
			E -= Inv * alphaBeta_c;
			std::cout << "ERROR1: " << E.norm() << std::endl;

#endif
      U.resize(0, 0);
      Vt.resize(0, 0);
      s.resize(0);
      delete[] work;

      // transfer inverse to blocks

      arrvec<int, 2> res_x;

      for (auto ix : xAB_idx) {
        for (auto iy : xAB_idx) {
          res_x[0].push_back(ix);
          res_x[1].push_back(iy);
        }
      }

      inv_local->reserve(res_x);

      dbcsr::iterator_t<2, double> iter_inv(*inv_local);
      iter_inv.start();

      while (iter_inv.blocks_left()) {
        iter_inv.next();

        int ix0 = iter_inv.idx()[0];
        int ix1 = iter_inv.idx()[1];

        int xoff0 = xAB_offs[xAB_mapping[ix0]];
        int xoff1 = xAB_offs[xAB_mapping[ix1]];

        int xsize0 = x[ix0];
        int xsize1 = x[ix1];

        Eigen::MatrixXd blk = Inv.block(xoff0, xoff1, xsize0, xsize1);

        bool found = true;
        double* blkptr = inv_local->get_block_p(iter_inv.idx(), found);

        std::copy(blk.data(), blk.data() + xsize0 * xsize1, blkptr);
      }

      iter_inv.stop();

      inv_local->filter(dbcsr::global::filter_eps);

      dbcsr::contract(1.0, *inv_local, *eri_local, 0.0, *c_xbb_AB)
          .filter(dbcsr::global::filter_eps)
          .perform("XY, YMN -> XMN");

#if 0	
			dbcsr::contract(-1.0, *met, *c_xbb_AB, 1.0, *eri_local)
				.filter(dbcsr::global::filter_eps)
				.perform("XY, YMN -> XMN");
				
			std::cout << "SUM" << std::endl;
			auto nblk = eri_local->num_blocks_total();
			std::cout << nblk << std::endl;
#endif

      dbcsr::copy(*c_xbb_AB, *c_xbb_local).move_data(true).sum(true).perform();

      eri_local->clear();
      inv_local->clear();
    };  // end worker function

    util::basic_scheduler worker(m_cart.comm(), ntasks, workfunc);
    worker.run();

    dbcsr::copy_local_to_global(*c_xbb_local, *c_xbb_global);

    c_xbb_batched->compress({inu}, c_xbb_global);
    c_xbb_local->clear();
    c_xbb_global->clear();

    task_off += ntasks;

  }  // end loop over batches

  c_xbb_batched->compress_finalize();

  TIME.finish();

  double occupation = c_xbb_batched->occupation() * 100;

  if (occupation > 100)
    throw std::runtime_error("Fitting coefficients occupation more than 100%");

  return c_xbb_batched;
}

}  // namespace ints

}  // namespace megalochem
