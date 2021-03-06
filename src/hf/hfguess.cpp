#include <dbcsr_conversions.hpp>
#include <limits>
#include "hf/hfmod.hpp"
#include "math/linalg/LLT.hpp"
#include "math/linalg/piv_cd.hpp"
#include "math/solvers/hermitian_eigen_solver.hpp"

#include <dbcsr_matrix_ops.hpp>

#include "extern/scalapack.hpp"

namespace megalochem {

namespace hf {

// Taken from PSI4
static const std::vector<int> reference_S = {
    0, 1, 0, 1, 0, 1, 2, 3, 2, 1, 0, 1, 0, 1, 2, 3, 2, 1, 0, 1, 0, 1,
    2, 3, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0, 1, 0, 1, 2, 5, 6, 5,
    4, 3, 0, 1, 0, 1, 2, 3, 2, 1, 0, 1, 0, 1, 0, 3, 4, 5, 6, 7, 8, 5,
    4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0};

// Also Taken from PSI4
static const std::vector<int> conf_orb = {0, 2, 10, 18, 36, 54, 86, 118};

/*
void scale_huckel(dbcsr::tensor<2>& t, std::vector<double>& v) {

        //does t[ij] = (v[i] + v[j])*t[ij]

        dbcsr::iterator_t<2> iter(t);

        auto blkoff = t.blk_offset();

        while (iter.blocks_left()) {

                iter.next();

                auto idx = iter.idx();
                auto sizes = iter.sizes();
                bool found = false;

                auto blk = t.get_block({.idx = idx, .blk_size = sizes, .found =
found});

                int off1 = blkoff[0][idx[0]];
                int off2 = blkoff[1][idx[1]];

                double prefac = 1.0;

                for (int j = 0; j != sizes[1]; ++j) {
                        for (int i = 0; i != sizes[0]; ++i) {

                                prefac = (off1 + i == off2 + j) ? 1.0 :
HF_GWH_K;

                                blk(i,j) *= 0.5 * prefac * (v[i + off1] + v[j +
off2]);
                }}

                t.put_block({.idx = idx, .blk = blk});

        }

}
*/
/*
void gram_schmidt(dbcsr::shared_matrix<double>& mat) {

        int nrows = mat->nfullrows_total();
        int ncols = mat->nfullcols_total();

        int nsplit = 10;

        auto V = dbcsr::matrix_to_scalapack(mat, "mat_s", nsplit, nsplit, 0, 0);

        scalapack::distmat<double> U(nrows, ncols, nsplit, nsplit, 0, 0);

        for (int icol = 0; icol != ncols; ++icol) {

                // u_n = v_n

                c_pdgeadd('N', nrows, 1, 1.0, V.data(), 0, icol,
V.desc().data(), 1.0, U.data(), 0, icol, U.desc().data());

                for (int jcol = 0; jcol != icol; ++jcol) {

                        // dot(u_jcol, v_icol)
                        double dot_uv = c_pddot(nrows, U.data(), 0, jcol,
U.desc().data(), 1, V.data(), 0, icol, V.desc().data(), 1);

                        // dot(u_jcol, u_jcol)
                        double dot_uu = c_pddot(nrows, U.data(), 0, jcol,
U.desc().data(), 1, U.data(), 0, jcol, U.desc().data(), 1);

                        std::cout << dot_uv << " " << dot_uu << std::endl;

                        // u_icol += - proj(u_jcol, v_icol)
                        c_pdgeadd('N', nrows, 1, -dot_uv/dot_uu, U.data(), 0,
jcol, U.desc().data(), 1.0, U.data(), 0, icol, U.desc().data());

                }

        }

        auto w = mat->get_cart();
        auto rblk = mat->row_blk_sizes();
        auto cblk = mat->col_blk_sizes();

        auto mat_ortho = dbcsr::scalapack_to_matrix(U, mat->name() + "_ortho",
w, rblk, cblk);

        //dbcsr::print(*mat);

        //dbcsr::print(*mat_ortho);

        mat = mat_ortho;

}*/

void hfmod::compute_guess()
{
  auto& t_guess = TIME.sub("Forming Guess");

  t_guess.start();

  if (m_guess == "core") {
    LOG.os<>("Forming guess from core...\n");

    // form density and coefficients by diagonalizing the core matrix
    // dbcsr::copy<2>({.t_in = *m_core_bb, .t_out = *m_f_bb_A});
    m_f_bb_A->copy_in(*m_core_bb);

    // dbcsr::print(*m_core_bb);
    // dbcsr::print(*m_f_bb_A);

    if (!m_restricted)
      m_f_bb_B->copy_in(*m_core_bb);

    diag_fock();
  }
  else if (m_guess == "SADNO" || m_guess == "SAD") {
    LOG.os<>("Forming guess from SAD :'( ...\n");
    // divide up comm <- for later

    // get mol info, check for atom types...
    std::vector<desc::Atom> atypes;

    for (auto atom1 : m_mol->atoms()) {
      auto found = std::find_if(
          atypes.begin(), atypes.end(), [&atom1](const desc::Atom& atom2) {
            return atom2.atomic_number == atom1.atomic_number;
          });

      if (found == atypes.end()) {
        atypes.push_back(atom1);
      }
    }

    // std::cout << "TYPES: " << std::endl;
    // for (auto a : atypes) {
    //	std::cout << a.atomic_number << std::endl;
    //}

    auto are_oncentre = [&](desc::Atom& atom, desc::Shell& shell) {
      double lim = std::numeric_limits<double>::epsilon();
      if (fabs(atom.x - shell.O[0]) < lim && fabs(atom.y - shell.O[1]) < lim &&
          fabs(atom.z - shell.O[2]) < lim)
        return true;

      return false;
    };

    std::map<int, Eigen::MatrixXd> locdensitymap;
    std::map<int, Eigen::MatrixXd> densitymap;

    // divide it up
    std::vector<desc::Atom> my_atypes;

    for (size_t I = 0; I != atypes.size(); ++I) {
      // if (m_cart.rank() == I % m_cart.size()) my_atypes.push_back(atypes[I]);
      // All om rank 1 for now
      if (m_cart.rank() == 0)
        my_atypes.push_back(atypes[I]);
    }

    if (LOG.global_plev() >= 1) {
      LOG.os<>("Distribution of atom types along processors:\n");
      for (int i = 0; i != m_cart.size(); ++i) {
        if (m_cart.rank() == i) {
          std::cout << "Rank " << m_cart.rank() << std::endl;
          for (auto e : my_atypes) { std::cout << e.atomic_number << " "; }
          std::cout << std::endl;
        }
        MPI_Barrier(m_cart.comm());
      }
    }

    // set up new grid
    world wself(MPI_COMM_SELF);

    for (size_t I = 0; I != my_atypes.size(); ++I) {
      auto atom = my_atypes[I];
      int Z = atom.atomic_number;

      int atprint = LOG.global_plev() - 2;
      int charge = 0;
      int mult = 0;  // will be overwritten

      std::vector<desc::Atom> atvec = {atom};

      std::vector<desc::Shell> at_vshell;

      // find basis functions
      for (auto& c : *m_mol->c_basis()) {
        for (auto& s : c.shells) {
          if (are_oncentre(atom, s))
            at_vshell.push_back(s);
        }
      }

      desc::shared_cluster_basis at_basis =
          std::make_shared<desc::cluster_basis>(at_vshell, "atomic", 1);

      desc::shared_cluster_basis at_dfbasis = nullptr;

      /*if (m_mol->c_dfbasis()) {
              //std::cout << "INSIDE HERE." << std::endl;
              std::vector<desc::Shell> at_libint_dfbasis;

              for (auto& c : *m_mol->c_dfbasis()) {
                      for (auto& s : c) {
                              if (are_oncentre(atom, s))
      at_libint_dfbasis.push_back(s);
                      }
              }

              at_dfbasis = std::make_shared<desc::cluster_basis>(
                      at_libint_dfbasis, "atomic", 1);;

      }*/

      std::string name =
          "ATOM_rank" + std::to_string(m_cart.rank()) + "_" + std::to_string(Z);

      desc::shared_molecule at_smol = desc::molecule::create()
                                          .comm(MPI_COMM_SELF)
                                          .name(name)
                                          .cluster_basis(at_basis)
                                          .atoms(atvec)
                                          .charge(charge)
                                          .mo_split(10)
                                          .mult(mult)
                                          .fractional(true)
                                          .spin_average(m_SAD_spin_average)
                                          .build();

      if (at_dfbasis)
        at_smol->set_cluster_dfbasis(at_dfbasis);

      if (LOG.global_plev() >= 2) {
        at_smol->print_info(LOG.global_plev());
      }

      auto athfmod = hf::hfmod::create()
                         .set_world(wself)
                         .set_molecule(at_smol)
                         .guess(m_SAD_guess)
                         .scf_threshold(m_SAD_scf_threshold)
                         .do_diis(m_SAD_do_diis)
                         .build_J("exact")
                         .build_K("exact")
                         .eris("core")
                         .imeds("core")
                         .print(atprint)
                         .build();

      LOG(m_cart.rank())
          .os<1>(
              "Starting Atomic UHF for atom nr. ", I, " on rank ",
              m_cart.rank(), '\n');
      auto at_wfn = athfmod->compute();
      LOG(m_cart.rank())
          .os<1>(
              "Done with Atomic UHF for atom nr. ", I, " on rank ",
              m_cart.rank(), "\n");

      auto coA = at_wfn->hf_wfn->c_bo_A();
      auto coB = at_wfn->hf_wfn->c_bo_B();

      auto b = at_smol->dims().b();
      auto pA = dbcsr::matrix<>::create()
                    .name("p_bb_A")
                    .set_cart(wself.dbcsr_grid())
                    .row_blk_sizes(b)
                    .col_blk_sizes(b)
                    .matrix_type(dbcsr::type::symmetric)
                    .build();

      auto pB = dbcsr::matrix<>::create_template(*pA).name("p_bb_B").build();

      dbcsr::multiply('N', 'T', 1.0, *coA, *coA, 0.0, *pA).perform();
      if (coB)
        dbcsr::multiply('N', 'T', 1.0, *coB, *coB, 0.0, *pB).perform();

      // std::cout << "PA on rank: " << myrank << std::endl;
      // dbcsr::print(*pA);

      if (coB) {
        auto pscaled = dbcsr::matrix<>::copy(*pA)
                           .name(at_smol->name() + "_density")
                           .build();
        pscaled->add(0.5, 0.5, *pB);
        locdensitymap[Z] = dbcsr::matrix_to_eigen(*pscaled);
      }
      else {
        locdensitymap[Z] = dbcsr::matrix_to_eigen(*pA);
      }

      // ints::registry INTS_REGISTRY;
      // INTS_REGISTRY.clear(name);
    }

    //}

    // MPI_Barrier(m_cart.comm());

    //}

    MPI_Barrier(m_world.comm());

    wself.free();

    MPI_Barrier(m_world.comm());

    // std::cout << "DISTRIBUTING" << std::endl;

    // distribute to other nodes
    for (int i = 0; i != m_cart.size(); ++i) {
      // std::cout << "LOOP: " << i << std::endl;

      if (i == m_cart.rank()) {
        int n = locdensitymap.size();

        // std::cout << "N: " << n << std::endl;

        MPI_Bcast(&n, 1, MPI_INT, i, m_cart.comm());

        for (auto& den : locdensitymap) {
          int Z = den.first;
          size_t size = den.second.size();

          // std::cout << "B1 " << Z << std::endl;
          MPI_Bcast(&Z, 1, MPI_INT, i, m_cart.comm());
          // std::cout << "B2 " << size << std::endl;
          MPI_Bcast(&size, 1, MPI_UNSIGNED, i, m_cart.comm());

          auto& mat = den.second;

          // std::cout << "B3" << std::endl;
          MPI_Bcast(mat.data(), size, MPI_DOUBLE, i, m_cart.comm());

          densitymap[Z] = mat;

          MPI_Barrier(m_cart.comm());
        }
      }
      else {
        int n = -1;

        MPI_Bcast(&n, 1, MPI_INT, i, m_cart.comm());

        for (int ni = 0; ni != n; ++ni) {
          size_t size = 0;
          int Z = 0;

          MPI_Bcast(&Z, 1, MPI_INT, i, m_cart.comm());
          MPI_Bcast(&size, 1, MPI_UNSIGNED, i, m_cart.comm());

          // std::cout << "Other rank: " << Z << " " << size << std::endl;

          Eigen::MatrixXd mat((int)sqrt(size), (int)sqrt(size));

          MPI_Bcast(mat.data(), size, MPI_DOUBLE, i, m_cart.comm());

          densitymap[Z] = mat;

          MPI_Barrier(m_cart.comm());
        }
      }

      MPI_Barrier(m_cart.comm());
    }

    size_t nbas = m_mol->c_basis()->nbf();

    Eigen::MatrixXd ptot_eigen = Eigen::MatrixXd::Zero(nbas, nbas);
    auto csizes = m_mol->dims().b();
    int off = 0;

    for (size_t i = 0; i != m_mol->atoms().size(); ++i) {
      int Z = m_mol->atoms()[i].atomic_number;
      auto& at_density = densitymap[Z];
      int at_nbas = at_density.rows();

      ptot_eigen.block(off, off, at_nbas, at_nbas) = densitymap[Z];

      off += at_nbas;
    }

    auto b = m_mol->dims().b();
    m_p_bb_A = dbcsr::eigen_to_matrix(
        ptot_eigen, m_cart, "p_bb_A", b, b, dbcsr::type::symmetric);

    if (m_guess == "SADNO") {
      LOG.os<>("Forming natural orbitals from SAD guess density.\n");

      math::hermitian_eigen_solver solver(
          m_world, m_p_bb_A, 'V', (LOG.global_plev() >= 2) ? true : false);

      m_SAD_rank = m_mol->c_basis()->nbf();
      auto m = dbcsr::split_range(m_SAD_rank, m_mol->mo_split());

      solver.eigvec_colblks(m);

      solver.compute();

      auto eigvals = solver.eigvals();
      m_c_bm_A = solver.eigvecs();

      std::for_each(eigvals.begin(), eigvals.end(), [](double& d) {
        d = (d < std::numeric_limits<double>::epsilon()) ? 0 : sqrt(d);
      });

      m_c_bm_A->scale(eigvals, "right");
    }
    else {
      LOG.os<>("Forming cholesky orbitals from SAD guess.");

      math::pivinc_cd cd(m_world, m_p_bb_A, LOG.global_plev());

      cd.compute();

      m_SAD_rank = cd.rank();

      auto m = dbcsr::split_range(m_SAD_rank, m_mol->mo_split());
      auto b = m_mol->dims().b();

      m_c_bm_A = cd.L(b, m);

      m_c_bm_A->setname("c_bm_A");
      m_c_bm_A->filter(dbcsr::global::filter_eps);

      // dbcsr::print(*m_c_bm_A);

      // dbcsr::multiply('N', 'T', *m_c_bm_A, *m_c_bm_A,
      // *m_p_bb_A).beta(-1.0).perform();

      // dbcsr::print(*m_p_bb_A);
    }

    m_p_bb_A->filter(dbcsr::global::filter_eps);

    if (!m_restricted) {
      if (!m_nobetaorb)
        m_c_bm_B->copy_in(*m_c_bm_A);
      m_p_bb_B->copy_in(*m_p_bb_A);
    }

    LOG.os<2>("SAD density matrices.\n");
    if (LOG.global_plev() >= 2) {
      dbcsr::print(*m_p_bb_A);
      dbcsr::print(*m_c_bm_A);
      if (m_p_bb_B) {
        dbcsr::print(*m_p_bb_B);
        dbcsr::print(*m_c_bm_B);
      }
    }

    LOG.os<>("Finished with SAD.\n");

    // end SAD :(
  }
  else if (m_guess == "project") {
    LOG.os<>("Launching Hartree Fock calculation with secondary basis\n");

    auto cbas2 = m_mol->c_basis2();
    if (!cbas2) {
      throw std::runtime_error("No secondary basis given!");
    }

    auto mol_sub = desc::molecule::create()
                       .comm(m_cart.comm())
                       .name("SUB")
                       .atoms(m_mol->atoms())
                       .cluster_basis(cbas2)
                       .charge(m_mol->charge())
                       .mult(m_mol->mult())
                       .mo_split(m_mol->mo_split())
                       .build();

    auto subhf = hfmod::create()
                     .set_world(m_world)
                     .set_molecule(mol_sub)
                     .df_basis(m_df_basis2)
                     .guess("SAD")
                     .scf_threshold(10 * m_scf_threshold)
                     .max_iter(m_max_iter)
                     .do_diis(m_do_diis)
                     .diis_max_vecs(m_diis_max_vecs)
                     .diis_min_vecs(m_diis_min_vecs)
                     .build_J(m_build_J)
                     .build_K(m_build_K)
                     .eris(m_eris)
                     .imeds(m_imeds)
                     .df_metric(m_df_metric)
                     .print(m_print)
                     .nbatches_b(m_nbatches_b)
                     .nbatches_x(m_nbatches_x)
                     .build();

    auto subwfn = subhf->compute();

    LOG.os<>("Finished Hartree Fock computation with secondary basis set.\n");
    LOG.os<>("Now computing projected MO coefficient matrices");

    auto b = m_mol->dims().b();
    auto b2 = m_mol->dims().b2();
    auto m = m_c_bm_A->col_blk_sizes();
    auto oa = m_mol->dims().oa();
    auto ob = m_mol->dims().ob();

    auto c_b2o_A = subwfn->hf_wfn->c_bo_A();
    auto c_b2o_B = subwfn->hf_wfn->c_bo_B();

    math::LLT lltsolver(m_world, m_s_bb, 0);
    lltsolver.compute();
    auto s_inv_bb = lltsolver.inverse(b);

    ints::aofactory aofac(m_mol, m_world);
    auto s_bb2 = aofac.ao_overlap2();

    // dbcsr::print(*s_bb2);

    auto x_bb2 = dbcsr::matrix<double>::create()
                     .set_cart(m_cart)
                     .name("x_bb2")
                     .row_blk_sizes(b)
                     .col_blk_sizes(b2)
                     .matrix_type(dbcsr::type::no_symmetry)
                     .build();

    dbcsr::multiply('N', 'N', 1.0, *s_inv_bb, *s_bb2, 0.0, *x_bb2).perform();

    auto c_bo_A = dbcsr::matrix<double>::create()
                      .set_cart(m_cart)
                      .name("c_bo_A")
                      .row_blk_sizes(b)
                      .col_blk_sizes(oa)
                      .matrix_type(dbcsr::type::no_symmetry)
                      .build();

    dbcsr::multiply('N', 'N', 1.0, *x_bb2, *c_b2o_A, 0.0, *c_bo_A).perform();

    // gram_schmidt(c_bo_A);

    // copy over

    auto copy = [&](auto& c_bo, auto& c_bm) {
      auto c_bo_eigen = dbcsr::matrix_to_eigen(*c_bo);
      int nrows = c_bm->nfullrows_total();
      int ncols = c_bm->nfullcols_total();
      Eigen::MatrixXd c_bm_eigen = Eigen::MatrixXd::Zero(nrows, ncols);
      c_bm_eigen.block(0, 0, c_bo_eigen.rows(), c_bo_eigen.cols()) = c_bo_eigen;
      c_bm = dbcsr::eigen_to_matrix(
          c_bm_eigen, m_cart, c_bm->name(), b, m, dbcsr::type::no_symmetry);
    };

    copy(c_bo_A, m_c_bm_A);

    // dbcsr::print(*c_bo_A);

    // dbcsr::print(*m_c_bm_A);

    dbcsr::multiply('N', 'T', 1.0, *c_bo_A, *c_bo_A, 0.0, *m_p_bb_A).perform();

    if (m_c_bm_B) {
      auto c_bo_B = dbcsr::matrix<double>::create()
                        .set_cart(m_cart)
                        .name("c_bo_B")
                        .row_blk_sizes(b)
                        .col_blk_sizes(ob)
                        .matrix_type(dbcsr::type::no_symmetry)
                        .build();

      dbcsr::multiply('N', 'N', 1.0, *x_bb2, *c_b2o_B, 0.0, *c_bo_B).perform();

      // gram_schmidt(c_bo_B);

      dbcsr::multiply('N', 'T', 1.0, *c_bo_B, *c_bo_B, 0.0, *m_p_bb_B)
          .perform();

      copy(c_bo_B, m_c_bm_B);
    }
  }
  else {
    throw std::runtime_error("Unknown option for guess: " + m_guess);
  }

  t_guess.finish();
}

}  // namespace hf

}  // namespace megalochem
