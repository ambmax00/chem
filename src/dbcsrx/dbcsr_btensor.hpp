#ifndef DBCSR_BTENSOR_HPP
#define DBCSR_BTENSOR_HPP

#ifndef TEST_MACRO
  #include <mpi.h>
  #include <cstdlib>
  #include <dbcsr_tensor_ops.hpp>
  #include <filesystem>
  #include <functional>
  #include <map>
  #include <memory>
  #include "utils/mpi_time.hpp"
  #include "utils/unique.hpp"
#endif

#include "utils/ppdirs.hpp"

//#define _CORE_VECTOR

namespace dbcsr {

enum class btype { core, disk, direct };

inline btype get_btype(std::string str)
{
  if (str == "core")
    return btype::core;
  if (str == "disk")
    return btype::disk;
  if (str == "direct")
    return btype::direct;
  throw std::runtime_error("Invalid btensor type.");
}

inline vec<vec<int>> make_blk_bounds(
    std::vector<int> blksizes,
    int nbatches,
    std::optional<std::vector<int>> blkmap = std::nullopt)
{
  int nblks = blksizes.size();
  int nele = std::accumulate(blksizes.begin(), blksizes.end(), 0);

  if (nblks < nbatches)
    nbatches = nblks;

  double nele_per_batch = (double)nele / (double)nbatches;

  if (blkmap && blksizes.size() != blkmap->size())
    throw std::runtime_error(
        "Block sizes and block map do not have the same dimension");

  vec<vec<int>> out;
  int current_sum = 0;
  int current_centre = -1;
  int next_centre = -1;
  int ibatch = 1;
  int first_blk = 0;
  int last_blk = 0;

  for (int i = 0; i != nblks; ++i) {
    current_sum += blksizes[i];
    current_centre = (blkmap) ? blkmap->at(i) : i;

    if (i != nblks - 1 && blkmap) {
      next_centre = blkmap->at(i + 1);
    }
    else if (i != nblks - 1 && !blkmap) {
      next_centre = i + 1;
    }
    else if (i == nblks - 1 && blkmap) {
      next_centre = -1;
    }
    else {
      next_centre = i + 1;
    }

    if (current_sum >= ibatch * nele_per_batch &&
        (next_centre != current_centre)) {
      last_blk = i;
      vec<int> b = {first_blk, last_blk};
      out.push_back(b);
      first_blk = i + 1;
      ++ibatch;
    }
  }

  // for (auto p : out) {
  //	std::cout << p[0] << " " << p[1] << std::endl;
  //}

  if (out.size() == 0)
    throw std::runtime_error("Block bounds are zero.");

  return out;
}

struct btensor_global {
  static inline bool error_flag = false;
};

template <
    int N,
    typename T = double,
    typename = typename std::enable_if<(N >= 3 && N <= 4)>::type>
class btensor {
 protected:
  MPI_Comm m_comm;
  util::mpi_log LOG;
  int m_mpirank;
  int m_mpisize;

  dbcsr::stensor<N, T> m_read_tensor;
  // tensor which keeps original dist for reading

  dbcsr::stensor<N, T> m_work_tensor;
  // underlying shared tensor for contraction/copying

#ifdef _CORE_VECTOR
  std::vector<dbcsr::stensor<N, T>> m_core_vector;
#endif

  std::string m_name;
  dbcsr::shared_pgrid<N> m_spgrid_N;
  arrvec<int, N> m_blk_sizes, m_blk_maps;

  std::string m_filename;
  std::string m_path;
  // root name for files

  btype m_type;

  /* ========== dynamic tensor variables ========== */

  int m_nblkloc = 0;
  // number of local blocks
  int m_nblkloc_global = 0;
  // number og local blocks in non-sparse tensor
  int m_nzeloc = 0;
  // number of local non-zero elements
  int64_t m_nblktot_global = 0;
  // total number of blocks in non-sparse tensor
  int64_t m_nblktot = 0;
  // total number of blocks
  int64_t m_nzetot = 0;
  // total number of non-zero elements

  /* ======== BATCHING INFO ========== */

  int m_write_current_batch;
  int m_read_current_batch;
  vec<int> m_read_current_dims;
  bool m_read_current_is_contiguous;

  bool m_is_compress_initialized, m_is_decompress_initialized;

  arrvec<vec<int>, N> m_blk_bounds;
  arrvec<vec<int>, N> m_bounds;
  arrvec<int, N> m_full_blk_bounds;
  arrvec<int, N> m_full_bounds;

  std::array<int, N> m_nbatches_dim;

  struct view {
    vec<int> dims;
    bool is_contiguous;
    int nbatches;
    vec<int> map1, map2;
    vec<vec<int>> nblksprocbatch;
    vec<vec<int>> nzeprocbatch;
    std::string file_name;
  };

  view m_wrview;
  std::map<vec<int>, view> m_rdviewmap;

  /* =========== FUNCTIONS ========== */

  using generator_type =
      std::function<void(dbcsr::stensor<N, T>&, vec<vec<int>>&)>;

  generator_type m_generator;

 public:
  btensor(MPI_Comm comm, int print) : LOG(comm, print)
  {
  }

#define BTENSOR_CREATE_LIST \
  (((shared_pgrid<N>), set_pgrid), ((arrvec<int, N>&), blk_sizes), \
   ((arrvec<int, N>&), blk_maps), ((std::array<int, N>), batch_dims), \
   ((std::string), name), ((dbcsr::btype), btensor_type), \
   ((util::optional<int>), print))

  MAKE_PARAM_STRUCT(create, BTENSOR_CREATE_LIST, ())
  MAKE_BUILDER_CLASS(btensor, create, BTENSOR_CREATE_LIST, ())

  btensor(create_pack&& p) :
      m_comm(p.p_set_pgrid->comm()),
      LOG(p.p_set_pgrid->comm(), (p.p_print) ? *p.p_print : 0), m_mpirank(-1),
      m_mpisize(-1), m_name(p.p_name), m_spgrid_N(p.p_set_pgrid),
      m_blk_sizes(p.p_blk_sizes), m_blk_maps(p.p_blk_maps), m_filename(),
      m_type(p.p_btensor_type), m_is_compress_initialized(false),
      m_is_decompress_initialized(false)
  {
    MPI_Comm_rank(m_comm, &m_mpirank);
    MPI_Comm_size(m_comm, &m_mpisize);

    LOG.os<1>("Setting up batch tensor information for ", m_name, +".\n");

    std::string path = std::filesystem::current_path();
    path += "/" + util::unique("mega_batchtensor", "", m_comm) + "/";
    if (m_mpirank == 0) {
      std::filesystem::create_directory(path);
    }

    m_path = path;
    m_filename = path + m_name + ".dat";

    // divide dimensions

    arrvec<int, N> blkoffsets;
    auto nbatches = p.p_batch_dims;

    for (int i = 0; i != N; ++i) {
      // offsets
      int off = 0;
      for (auto blksize : m_blk_sizes[i]) {
        blkoffsets[i].push_back(off);
        off += blksize;
      }

      // batch bounds
      m_blk_bounds[i] =
          make_blk_bounds(m_blk_sizes[i], nbatches[i], m_blk_maps[i]);

      m_bounds[i] = m_blk_bounds[i];
      auto& ibds = m_bounds[i];
      auto& iblkoffs = blkoffsets[i];
      auto& iblksizes = m_blk_sizes[i];

      for (size_t x = 0; x != ibds.size(); ++x) {
        ibds[x][0] = iblkoffs[ibds[x][0]];
        ibds[x][1] = iblkoffs[ibds[x][1]] + iblksizes[ibds[x][1]] - 1;
      }

      // full bounds

      int nfull_blk = m_blk_sizes[i].size();
      m_full_blk_bounds[i] = vec<int>{0, nfull_blk - 1};

      int nfull =
          std::accumulate(m_blk_sizes[i].begin(), m_blk_sizes[i].end(), 0);
      m_full_bounds[i] = vec<int>{0, nfull - 1};

      // nbatches

      m_nbatches_dim[i] = m_blk_bounds[i].size();
    }

    LOG.os<1>("Batch bounds: \n");
    for (int i = 0; i != N; ++i) {
      LOG.os<1>("Dimension ", i, ":\n");
      for (auto b : m_blk_bounds[i]) { LOG.os<1>(b[0], " -> ", b[1], " "); }
      LOG.os<1>('\n');
    }

    if (m_type == btype::disk) {
      create_file();
    }

    LOG.os<1>("Finished setting up batchtensor ", m_name, ".\n");

    reset_var();
  }

  btensor(const btensor& in) = delete;

#define BTENSOR_TEMPLATE_ILIST (((btensor<N, T>&), t_in))

#define BTENSOR_TEMPLATE_PLIST \
  (((std::string), name), ((dbcsr::btype), btensor_type), \
   ((util::optional<int>), print))

  MAKE_PARAM_STRUCT(
      create_template, BTENSOR_TEMPLATE_PLIST, BTENSOR_TEMPLATE_ILIST)
  MAKE_BUILDER_CLASS(
      btensor, create_template, BTENSOR_TEMPLATE_PLIST, BTENSOR_TEMPLATE_ILIST)

  btensor(create_template_pack&& p) :
      m_comm(p.p_t_in.m_comm),
      LOG(p.p_t_in.m_comm, (p.p_print) ? *p.p_print : 0),
      m_mpirank(p.p_t_in.m_mpirank), m_mpisize(p.p_t_in.m_mpisize),
      m_name(p.p_name), m_spgrid_N(p.p_t_in.m_spgrid_N),
      m_blk_sizes(p.p_t_in.m_blk_sizes), m_blk_maps(p.p_t_in.m_blk_maps),
      m_filename(), m_type(p.p_btensor_type), m_is_compress_initialized(false),
      m_is_decompress_initialized(false)
  {
    LOG.os<1>("Setting up batch tensor information for ", m_name, +".\n");

    m_path = util::unique("mega_batchtensor", "", m_comm) + "/";
    if (m_mpirank == 0) {
      std::filesystem::create_directory(m_path);
    }
    m_filename = m_path + m_name + ".dat";
    m_blk_bounds = p.p_t_in.m_blk_bounds;
    m_bounds = p.p_t_in.m_bounds;
    m_full_blk_bounds = p.p_t_in.m_full_blk_bounds;
    m_full_bounds = p.p_t_in.m_full_bounds;
    m_nbatches_dim = p.p_t_in.m_nbatches_dim;

    MPI_Barrier(m_comm);

    if (m_type == btype::disk) {
      create_file();
    }

    LOG.os<1>("Finished setting up batchtensor ", m_name, ".\n");

    reset_var();
  }

  void set_generator(generator_type& func)
  {
    m_generator = func;
  }

  /* Create all necessary files.
   * !!! Deletes previous files and resets all variables !!! */
  void create_file()
  {
    delete_file();

    MPI_File fh;

    LOG.os<1>("Creating files for ", m_filename, '\n');

    MPI_File_open(
        m_comm, m_filename.c_str(), MPI_MODE_CREATE | MPI_MODE_WRONLY,
        MPI_INFO_NULL, &fh);

    MPI_File_close(&fh);
  }

  /* Deletes all files asscoiated with tensor.
   * !!! Resets variables !!! */
  void delete_file()
  {
    LOG.os<1>("Deleting files for ", m_filename, '\n');
    MPI_File_delete(m_filename.c_str(), MPI_INFO_NULL);
  }

  void reset_var()
  {
    m_nblkloc_global = 0;
    m_nblktot = 0;
    m_nblkloc = 0;
    m_nzeloc = 0;
    m_nblktot = 0;
    m_nzetot = 0;

    m_wrview.nblksprocbatch.clear();
    m_wrview.nzeprocbatch.clear();

    MPI_File_delete(m_wrview.file_name.c_str(), MPI_INFO_NULL);

    for (auto& rview : m_rdviewmap) {
      MPI_File_delete(rview.second.file_name.c_str(), MPI_INFO_NULL);
    }
    m_rdviewmap.clear();
  }

  void reset()
  {
    if (m_work_tensor)
      m_work_tensor->clear();
    if (m_read_tensor)
      m_read_tensor->clear();
    reset_var();
    if (m_type == btype::disk) {
      delete_file();
      create_file();
    }
  }

  ~btensor()
  {
    if (m_type == btype::disk)
      delete_file();
    reset_var();
    if (m_mpirank == 0) {
      LOG.os<1>("Removing ", m_name, " in ", m_path, '\n');
      std::filesystem::remove(m_path);
    }
  }

  int flatten(vec<int>& idx, vec<int>& dims)
  {
    vec<int> bsizes(dims.size());
    for (size_t i = 0; i != bsizes.size(); ++i) {
      bsizes[i] = m_nbatches_dim[dims[i]];
    }

    int flat_idx = 0;

    for (size_t i = 0; i != bsizes.size(); ++i) {
      int off = 1;
      for (size_t n = bsizes.size() - 1; n > i; --n) { off *= bsizes[i]; }
      flat_idx += idx[i] * off;
    }

    return flat_idx;
  }

  int get_nbatches(vec<int> dims)
  {
    /* get total number of batches */
    int nbatches = 1;
    for (size_t i = 0; i != dims.size(); ++i) {
      int idx = dims[i];
      nbatches *= m_nbatches_dim[idx];
    }
    return nbatches;
  }

  // check whether batch exceeds 2 GB per process
  bool fits_in_mem(std::vector<int> dims)
  {
    // get total number of batches
    int64_t ntot = 1;
    for (int ii = 0; ii != N; ++ii) {
      ntot *= std::accumulate(
          m_blk_sizes[ii].begin(), m_blk_sizes[ii].end(), (int64_t)0);
    }

    // get number of batches
    int nbatches = get_nbatches(dims);

    int64_t ntot_per_proc_batch =
        (ntot / (int64_t)m_mpisize) / (int64_t)nbatches;

    if (ntot_per_proc_batch * sizeof(T) >= 2 * 1e+9) {
      if (!btensor_global::error_flag) {
        LOG.os<>("!!!!!!!! WARNING !!!!!!!! \n"
                 "Batch sizes might be too large for this or other tensors!\n"
                 "This can lead to failure at random points in the program,\n"
                 "or give wrong results if it completes!\n"
                 "Either increase number of batches,\n"
                 "or increase number of MPI processes.\n");
      }
      btensor_global::error_flag = true;
      return false;
    }

    return true;
  }

  void compress_init(
      std::initializer_list<int> dim_list, vec<int> map1, vec<int> map2)
  {
    LOG.os<1>("Initializing compression for ", m_name, "...\n");

    vec<int> dims = dim_list;

    fits_in_mem(dims);

    m_wrview.nbatches = 1;
    m_wrview.dims = dims;
    m_wrview.is_contiguous = true;
    m_wrview.map1 = map1;
    m_wrview.map2 = map2;

    m_wrview.nbatches = get_nbatches(dims);

    LOG.os<1>("Total batches for writing: ", m_wrview.nbatches, '\n');

    reset_var();

    if (m_type == btype::core) {
#ifdef _CORE_VECTOR
      LOG.os<1>("Allocating core work tensors.\n");
      m_core_vector.resize(m_wrview.nbatches);
      for (auto& t : m_core_vector) {
        t = tensor<N, T>::create()
                .name(m_name + "_work")
                .set_pgrid(*m_spgrid_N)
                .map1(map1)
                .map2(map2)
                .blk_sizes(m_blk_sizes)
                .build();
      }

#else
      LOG.os<1>("Allocating core work tensor.\n");
      m_work_tensor = tensor<N, T>::create()
                          .name(m_name + "_work")
                          .set_pgrid(*m_spgrid_N)
                          .map1(map1)
                          .map2(map2)
                          .blk_sizes(m_blk_sizes)
                          .build();
#endif

      // m_work_tensor->batched_contract_init();
    }

    m_is_compress_initialized = true;

    LOG.os<1>("Finished initializing compression for ", m_name, "...\n");
  }

  vec<vec<int>> get_bounds(vec<int> idx, vec<int> dims)
  {
    vec<vec<int>> b(N);

    for (size_t i = 0; i != idx.size(); ++i) {
      b[dims[i]] = this->bounds(dims[i], idx[i]);
    }

    LOG.os<1>("Copy bounds.\n");
    for (int i = 0; i != N; ++i) {
      auto iter = std::find(dims.begin(), dims.end(), i);
      b[i] = (iter == dims.end()) ? full_bounds(i) : b[i];
      LOG.os<1>(b[i][0], " -> ", b[i][1], '\n');
    }

    return b;
  }

  vec<vec<int>> get_blk_bounds(vec<int> idx, vec<int> dims)
  {
    vec<vec<int>> b(N);

    for (size_t i = 0; i != idx.size(); ++i) {
      b[dims[i]] = this->blk_bounds(dims[i], idx[i]);
    }

    LOG.os<1>("Copy bounds.\n");
    for (int i = 0; i != N; ++i) {
      auto iter = std::find(dims.begin(), dims.end(), i);
      b[i] = (iter == dims.end()) ? full_blk_bounds(i) : b[i];
      LOG.os<1>(b[i][0], " -> ", b[i][1], '\n');
    }

    return b;
  }

  /* ... */
  void compress(std::initializer_list<int> idx_list, stensor<N, T> tensor_in)
  {
    LOG.os<1>("Compressing into ", m_name, "...\n");

    if (!m_is_compress_initialized) {
      throw std::runtime_error("Compression not initialized.\n");
    }

    if (m_is_decompress_initialized) {
      throw std::runtime_error(
          "Compressing, but decompression is initailized\n");
    }

    vec<int> idx = idx_list;

    auto map1 = tensor_in->map1_2d();
    auto map2 = tensor_in->map2_2d();

    if (map1 != m_wrview.map1 || map2 != m_wrview.map2) {
      throw std::logic_error("Batchtensor compression: incompatible maps.\n");
    }

    switch (m_type) {
      case btype::disk:
        compress_disk(idx, tensor_in);
        break;
      case btype::core:
        compress_core(idx, tensor_in);
        break;
      case btype::direct:
        compress_direct(tensor_in);
        break;
    }

    LOG.os<1>("Finished compression for ", m_name, "...\n");
  }

  void compress_direct(stensor<N, T> tensor_in)
  {
    tensor_in->clear();
    return;
  }

  void compress_core(vec<int> idx, stensor<N, T> tensor_in)
  {
    LOG.os<1>("Compressing into core memory...\n");

    auto b = get_bounds(idx, m_wrview.dims);

    LOG.os<1>("Copying\n");

    m_nzetot += tensor_in->num_nze_total();

#ifdef _CORE_VECTOR
    LOG.os<1>("Index: ", ibatch, '\n');
    copy(*tensor_in, *m_core_vector[ibatch])
        .bounds(b)
        .move_data(true)
        .perform();
#else
    copy(*tensor_in, *m_work_tensor)
        .bounds(b)
        .move_data(true)
        .sum(true)
        .perform();
#endif

    LOG.os<1>("DONE.\n");
  }

  void compress_disk(vec<int> idx, stensor<N, T> tensor_in)
  {
    auto dims = m_wrview.dims;
    int ibatch = flatten(idx, dims);
    int nbatches = m_wrview.nbatches;

    auto write_tensor = tensor_in;

    // writes the local blocks of batch ibatch to file
    // should only be called in order
    // blocks of a batch are stored as follows:

    // (batch1)                    (batch2)                ....
    // (blks node1)(blks node2)....(blks node1)(blks node2)....

    // allocate data

    LOG.os<1>("Writing data of tensor ", m_filename, " to file.\n");
    LOG.os<1>("Batch ", ibatch, '\n');

    int nze = write_tensor->num_nze();
    int nblocks = write_tensor->num_blocks();

    LOG.os<1>("NZE/NBLOCKS: ", nze, "/", nblocks, '\n');

    auto& nblksprocbatch = m_wrview.nblksprocbatch;
    auto& nzeprocbatch = m_wrview.nzeprocbatch;

    nblksprocbatch.resize(nbatches);
    nzeprocbatch.resize(nbatches);

    nblksprocbatch[ibatch] = vec<int>(m_mpisize);
    nzeprocbatch[ibatch] = vec<int>(m_mpisize);

    // communicate nzes + blks to other processes

    LOG.os<1>("Gathering nze and nblocks...\n");

    MPI_Allgather(
        &nze, 1, MPI_INT, nzeprocbatch[ibatch].data(), 1, MPI_INT, m_comm);
    MPI_Allgather(
        &nblocks, 1, MPI_INT, nblksprocbatch[ibatch].data(), 1, MPI_INT,
        m_comm);

    int64_t nblktotbatch = std::accumulate(
        nblksprocbatch[ibatch].begin(), nblksprocbatch[ibatch].end(),
        int64_t(0));

    int64_t nzetotbatch = std::accumulate(
        nzeprocbatch[ibatch].begin(), nzeprocbatch[ibatch].end(), int64_t(0));

    m_nblkloc += nblksprocbatch[ibatch][m_mpirank];
    m_nzeloc += nzeprocbatch[ibatch][m_mpirank];

    m_nblktot += nblktotbatch;

    m_nzetot += nzetotbatch;

    LOG(-1).os<1>("Local number of blocks: ", m_nblkloc, '\n');
    LOG.os<1>("Total number of blocks: ", nblktotbatch, '\n');
    LOG.os<1>("Total number of nze: ", nzetotbatch, '\n');

    // read blocks

    LOG.os<1>("Writing blocks...\n");

    dbcsr::iterator_t<N, T> iter(*write_tensor);

    iter.start();

    std::vector<MPI_Aint> blkoffbatch(
        nblocks);  // block offsets in file for this batch
    arrvec<int, N> blkidxbatch;  // block indices for this batch
    blkidxbatch.fill(vec<int>(nblocks));

    MPI_Aint offset = 0;

    int iblk = 0;

    while (iter.blocks_left()) {
      iter.next();

      auto& size = iter.size();
      auto& idx = iter.idx();

      for (int i = 0; i != N; ++i) { blkidxbatch[i][iblk] = idx[i]; }

      int ntot =
          std::accumulate(size.begin(), size.end(), 1, std::multiplies<int>());

      blkoffbatch[iblk++] = offset;

      offset += ntot;
    }

    iter.stop();

    // filenames

    std::string data_fname = m_filename;

    LOG.os<1>("Computing offsets...\n");

    // offsets
    MPI_Offset data_batch_offset = 0;

    int64_t nblkprev = 0;
    int64_t ndataprev = 0;

    // global batch offset
    for (int i = 0; i != ibatch; ++i) {
      ndataprev += std::accumulate(
          nzeprocbatch[i].begin(), nzeprocbatch[i].end(), int64_t(0));

      nblkprev += std::accumulate(
          nblksprocbatch[i].begin(), nblksprocbatch[i].end(), int64_t(0));
    }

    // local processor dependent offset
    for (int i = 0; i < m_mpirank; ++i) {
      ndataprev += nzeprocbatch[ibatch][i];
      nblkprev += nblksprocbatch[ibatch][i];
    }

    data_batch_offset = ndataprev;

    // add it to blkoffsets
    for (auto& off : blkoffbatch) { off += data_batch_offset; }

    // write indices and offsets to file
    std::string idx_filename = m_path + m_name + "_idx_write.dat";

    // std::cout << "IDX FILENAME: " << idx_filename << std::endl;

    MPI_File fh_info;
    MPI_File_open(
        m_comm, idx_filename.c_str(), MPI_MODE_WRONLY | MPI_MODE_CREATE,
        MPI_INFO_NULL, &fh_info);

    MPI_Offset idx_offset = nblkprev * (sizeof(int) * N + sizeof(MPI_Offset));
    for (int i = 0; i != N; ++i) {
      MPI_File_write_at_all(
          fh_info, idx_offset, blkidxbatch[i].data(), nblocks, MPI_INT,
          MPI_STATUS_IGNORE);
      idx_offset += (MPI_Offset)nblocks * sizeof(int);
    }
    MPI_File_write_at_all(
        fh_info, idx_offset, blkoffbatch.data(), nblocks, MPI_OFFSET,
        MPI_STATUS_IGNORE);

    m_wrview.file_name = idx_filename;

    MPI_File_close(&fh_info);

    // write data to file

    LOG.os<1>("Writing tensor data...\n");

    long long int datasize;
    T* data = write_tensor->data(datasize);

    MPI_File fh_data;

    MPI_File_open(
        m_comm, data_fname.c_str(), MPI_MODE_WRONLY, MPI_INFO_NULL, &fh_data);

    MPI_File_write_at_all(
        fh_data, data_batch_offset * sizeof(T), data, nze, MPI_DOUBLE,
        MPI_STATUS_IGNORE);

    MPI_File_close(&fh_data);

    if (LOG.global_plev() >= 10) {
      std::cout << "LOC BLK IDX AND OFFSET." << std::endl;
      for (size_t i = 0; i != blkidxbatch[0].size(); ++i) {
        for (int j = 0; j != N; ++j) { std::cout << blkidxbatch[j][i] << " "; }
        std::cout << blkoffbatch[i] << std::endl;
      }
    }

    write_tensor->clear();

    LOG.os<1>("Done with batch ", ibatch, '\n');
  }

  void compress_finalize()
  {
    LOG.os<1>("Finalizing compression for ", m_name, "...\n");

    // if (m_type == dbcsr::btype::core)
    // m_work_tensor->batched_contract_finalize();

    m_is_compress_initialized = false;
  }

  /*
  void reorder(vec<int> map1, vec<int> map2) {

          auto this_map1 = m_stensor->map1_2d();
          auto this_map2 = m_stensor->map2_2d();

          std::cout << "MAP1 vs MAP1 new" << std::endl;
          auto prt = [](vec<int> m) {
                  for (auto i : m) {
                          std::cout << i << " ";
                  } std::cout << std::endl;
          };

          prt(this_map1);
          prt(map1);

          std::cout << "MAP2 vs MAP2 new" << std::endl;
          prt(this_map2);
          prt(map2);


          if (map1 == this_map1 && map2 == this_map2) return;

          //std::cout << "REO" << std::endl;

          stensor<N,T> newtensor = tensor_create_template<N,T>(m_stensor)
                  .name(m_stensor->name()).map1(map1).map2(map2).build();

          if (m_type == btype::core) {
                  dbcsr::copy(*m_stensor, *newtensor).move_data(true).perform();
          }

          m_stensor = newtensor;

  }

  void reorder(shared_tensor<N,T> mytensor) {

          stensor<N,T> newtensor = tensor_create_template<N,T>(mytensor)
                  .name(m_stensor->name()).build();

          if (m_type == btype::core) {
                  dbcsr::copy(*m_stensor, *newtensor).move_data(true).perform();
          }

          m_stensor = newtensor;

  }*/

  view set_view(vec<int> dims)
  {
    view rview;

    rview.is_contiguous = false;
    rview.dims = dims;

    auto w_dims = m_wrview.dims;

    // get idx speed for writing
    auto map1 = m_wrview.map1;
    auto map2 = m_wrview.map2;

    map2.insert(map2.end(), map1.begin(), map1.end());

    auto order = map2;

    std::reverse(order.begin(), order.end());

    for (auto i : order) { LOG.os<1>(i, " "); }
    LOG.os<1>('\n');

    // compute super indices

    arrvec<int, N> rlocblkidx, wlocblkidx;
    vec<MPI_Offset> rlocblkoff, wlocblkoff;

    int nblkloctot = 0;

    for (int ibatch = 0; ibatch != m_wrview.nbatches; ++ibatch) {
      nblkloctot += m_wrview.nblksprocbatch[ibatch][m_mpirank];
    }

    for (auto& a : wlocblkidx) { a.resize(nblkloctot); }
    wlocblkoff.resize(nblkloctot);

    MPI_File fh_info;
    MPI_File_open(
        m_comm, m_wrview.file_name.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL,
        &fh_info);

    MPI_Offset idx_offset = 0;
    size_t loc_offset = 0;

    for (int ibatch = 0; ibatch != m_wrview.nbatches; ++ibatch) {
      for (int r = 0; r != m_mpisize; ++r) {
        int nblk = m_wrview.nblksprocbatch[ibatch][r];

        for (int i = 0; i != N; ++i) {
          if (r == m_mpirank) {
            MPI_File_read_at_all(
                fh_info, idx_offset, wlocblkidx[i].data() + loc_offset, nblk,
                MPI_INT, MPI_STATUS_IGNORE);
          }

          idx_offset += nblk * sizeof(int);
        }

        if (r == m_mpirank) {
          MPI_File_read_at_all(
              fh_info, idx_offset, wlocblkoff.data() + loc_offset, nblk,
              MPI_OFFSET, MPI_STATUS_IGNORE);
          loc_offset += nblk;
        }

        idx_offset += nblk * sizeof(MPI_Offset);
      }
    }

    MPI_File_close(&fh_info);

    if (LOG.global_plev() >= 10) {
      for (int m = 0; m != m_mpisize; ++m) {
        if (m == m_mpirank) {
          std::cout << "RANK " << m << std::endl;
          for (auto a : wlocblkidx) {
            for (auto e : a) { std::cout << e << " "; }
            std::cout << std::endl;
          }
          for (auto e : wlocblkoff) { std::cout << e << " "; }
          std::cout << std::endl;
        }
        MPI_Barrier(m_comm);
      }
    }

    // prepare

    vec<size_t> perm(wlocblkidx[0].size());
    vec<size_t> batchidx(wlocblkidx[0].size());

    std::iota(perm.begin(), perm.end(), (size_t)0);

    // start computing superindices
    std::array<int, N> bsizes;
    for (int i = 0; i != N; ++i) {
      bsizes[i] = ((int)dims.size() > i) ? m_nbatches_dim[dims[i]] : 1;
    }
    int nbatches = std::accumulate(
        bsizes.begin(), bsizes.end(), 1, std::multiplies<int>());

    LOG.os<1>("NBATCHES: ", nbatches, '\n');

    rview.nbatches = nbatches;

    auto& sizes = m_blk_sizes;
    arrvec<int, N> off = sizes;
    std::array<int, N> full;

    for (int i = 0; i != N; ++i) {
      int n = 0;
      for (size_t iblk = 0; iblk != sizes[i].size(); ++iblk) {
        off[i][iblk] = n;
        n += sizes[i][iblk];
        full[i] += sizes[i][iblk];
      }
    }

    auto& nblksprocbatch = rview.nblksprocbatch;
    auto& nzeprocbatch = rview.nzeprocbatch;

    nblksprocbatch.resize(nbatches);
    nzeprocbatch.resize(nbatches);
    for (int i = 0; i != nbatches; ++i) {
      nblksprocbatch[i].resize(m_mpisize);
      nzeprocbatch[i].resize(m_mpisize);
    }

    for (size_t iblk = 0; iblk != batchidx.size(); ++iblk) {
      // std::cout << "IBLK: " << iblk << std::endl;

      // get block index
      std::array<int, N> blk_idx;
      for (int i = 0; i != N; ++i) { blk_idx[i] = wlocblkidx[i][iblk]; }

      // compute batch indices
      std::array<int, N> bidx;
      for (int i = 0; i != N; ++i) {
        if (bsizes[i] == 1) {
          bidx[i] = 0;
          continue;
        }

        int idim = dims[i];
        for (int ibatch = 0; ibatch != m_nbatches_dim[idim]; ++ibatch) {
          if (blk_idx[idim] >= m_blk_bounds[idim][ibatch][0] &&
              blk_idx[idim] <= m_blk_bounds[idim][ibatch][1])
            bidx[i] = ibatch;
        }
      }

      // compute flattened batch index and super index
      size_t sbatch_idx = 0;

      switch (N) {
        case 3: {
          sbatch_idx =
              bidx[0] * bsizes[1] * bsizes[2] + bidx[1] * bsizes[2] + bidx[2];
          break;
        }
        case 4: {
          sbatch_idx = bidx[0] * bsizes[1] * bsizes[2] * bsizes[3] +
              bidx[1] * bsizes[2] * bsizes[3] + bidx[2] * bsizes[3] + bidx[3];
          break;
        }
      }

      // compute block size
      int blksize = 1;
      for (int i = 0; i != N; ++i) { blksize *= sizes[i][blk_idx[i]]; }

      nblksprocbatch[sbatch_idx][m_mpirank] += 1;
      nzeprocbatch[sbatch_idx][m_mpirank] += blksize;

      batchidx[iblk] = sbatch_idx;
    }

    // communicate
    LOG.os<1>("Gathering nze and nblocks...\n");

    for (int ibatch = 0; ibatch != nbatches; ++ibatch) {
      int nblk = nblksprocbatch[ibatch][m_mpirank];
      int nze = nzeprocbatch[ibatch][m_mpirank];

      MPI_Allgather(
          &nze, 1, MPI_INT, nzeprocbatch[ibatch].data(), 1, MPI_INT, m_comm);
      MPI_Allgather(
          &nblk, 1, MPI_INT, nblksprocbatch[ibatch].data(), 1, MPI_INT, m_comm);
    }

    if (LOG.global_plev() >= 10 && m_mpirank == 0) {
      std::cout << "BATCHSIZES:" << std::endl;
      for (size_t i = 0; i != nblksprocbatch.size(); ++i) {
        std::cout << "BATCH " << i << " " << nblksprocbatch[i][0] << std::endl;
      }
    }

    // sort the indices such that they are grouped in batches, but
    // offsets are in increasing order within batches
    std::sort(perm.begin(), perm.end(), [&](size_t i0, size_t i1) {
      return (batchidx[i0] != batchidx[i1]) ? (batchidx[i0] < batchidx[i1]) :
                                              (wlocblkoff[i0] < wlocblkoff[i1]);
    });

    rlocblkidx = wlocblkidx;
    rlocblkoff = wlocblkoff;

    for (size_t i = 0; i != perm.size(); ++i) {
      for (int n = 0; n != N; ++n) {
        rlocblkidx[n][i] = wlocblkidx[n][perm[i]];
      }
      rlocblkoff[i] = wlocblkoff[perm[i]];
    }

    if (LOG.global_plev() >= 10) {
      for (auto a : rlocblkidx) {
        for (auto i : a) { LOG.os<1>(i, " "); }
        LOG.os<1>('\n');
      }
      for (auto a : rlocblkoff) { LOG.os<1>(a, " "); }
      LOG.os<1>('\n');
    }

    std::string suffix;
    for (auto v : dims) { suffix += std::to_string(v); }

    rview.file_name = m_path + m_name + "_idx_read_" + suffix + ".dat";

    // std::cout << "FILENAME READ: " << rview.file_name << std::endl;

    MPI_File fh_read;
    MPI_File_open(
        m_comm, rview.file_name.c_str(), MPI_MODE_WRONLY | MPI_MODE_CREATE,
        MPI_INFO_NULL, &fh_read);

    idx_offset = 0;
    loc_offset = 0;

    for (int ibatch = 0; ibatch != rview.nbatches; ++ibatch) {
      for (int r = 0; r != m_mpisize; ++r) {
        int nblk = rview.nblksprocbatch[ibatch][r];

        for (int i = 0; i != N; ++i) {
          if (r == m_mpirank) {
            MPI_File_write_at_all(
                fh_read, idx_offset, rlocblkidx[i].data() + loc_offset, nblk,
                MPI_INT, MPI_STATUS_IGNORE);
          }

          idx_offset += nblk * sizeof(int);
        }

        if (r == m_mpirank) {
          MPI_File_write_at_all(
              fh_read, idx_offset, rlocblkoff.data() + loc_offset, nblk,
              MPI_OFFSET, MPI_STATUS_IGNORE);
          loc_offset += nblk;
        }

        idx_offset += nblk * sizeof(MPI_Offset);
      }
    }

    MPI_File_close(&fh_read);

    rview.map1 = map1;
    rview.map2 = map2;

    return rview;
  }

  void decompress_init(
      std::initializer_list<int> dims_list, vec<int> map1, vec<int> map2)
  {
    vec<int> dims = dims_list;

    fits_in_mem(dims);

    LOG.os<1>("Initializing decompression for ", m_name, "...\n");

    std::string vstr;
    for (auto i : map1) { vstr += std::to_string(i); }
    vstr += "|";
    for (auto i : map2) { vstr += std::to_string(i); }

    LOG.os<1>("Requested view: ", vstr, '\n');

    m_read_current_is_contiguous = (dims == m_wrview.dims) ? true : false;
    m_read_current_dims = dims;

    if (m_type == btype::core) {
#ifdef _CORE_VECTOR

      if (m_wrview.dims != dims) {
        LOG.os<1>("Reorganizing core tensors.\n");

        std::vector<dbcsr::shared_tensor<N, T>> newcorevec;

        newcorevec.resize(get_nbatches(dims));

        for (auto& t : newcorevec) {
          t = tensor<N, T>::create_template(*m_core_vector[0])
                  .map1(map1)
                  .map2(map2)
                  .name(m_name + "_work_core")
                  .build();

          t->batched_contract_init();
        }

        for (auto& oldt : m_core_vector) {
          if (dims.size() == 1) {
            const int d0 = dims[0];

            for (int ib0 = 0; ib0 != m_nbatches_dim[d0]; ++ib0) {
              vec<vec<int>> bds(N);
              for (int ii = 0; ii != N; ++ii) {
                bds[ii] = (ii == d0) ? this->bounds(ii, ib0) :
                                       bds[ii] = this->full_bounds(ii);
              }
              copy(*oldt, *newcorevec[ib0]).bounds(bds).sum(true).perform();
            }
          }
          else if (dims.size() == 2) {
            const int d0 = dims[0];
            const int d1 = dims[1];

            for (int ib0 = 0; ib0 != m_nbatches_dim[d0]; ++ib0) {
              for (int ib1 = 0; ib1 != m_nbatches_dim[d1]; ++ib1) {
                vec<vec<int>> bds(N);
                for (int ii = 0; ii != N; ++ii) {
                  if (ii == d0) {
                    bds[ii] = this->bounds(ii, ib0);
                  }
                  else if (ii == d1) {
                    bds[ii] = this->bounds(ii, ib1);
                  }
                  else {
                    bds[ii] = this->full_bounds(ii);
                  }
                }  // endfor ii
                std::vector<int> idx = {ib0, ib1};
                int idxflat = flatten(idx, dims);
                copy(*oldt, *newcorevec[idxflat])
                    .bounds(bds)
                    .sum(true)
                    .perform();

              }  // endfor ib1
            }  // endfor ib0
          }
          else {
            throw std::runtime_error("Batchtensor: nbatchdims > 3 NYI.");

          }  // end elseif

          oldt->clear();
          oldt->destroy();

        }  // endfor

        for (auto& t : newcorevec) { t->batched_contract_finalize(); }

        m_core_vector = std::move(newcorevec);

        // update
        m_wrview.dims = dims;
        m_wrview.map1 = map1;
        m_wrview.map2 = map2;
        m_wrview.nbatches = get_nbatches(dims);
      }
      else if (m_wrview.map1 != map1 || m_wrview.map2 != map2) {
        LOG.os<1>("Reordering core tensors.\n");

        std::vector<dbcsr::shared_tensor<N, T>> newcorevec;
        newcorevec.resize(m_core_vector.size());

        for (int ii = 0; ii != m_core_vector.size(); ++ii) {
          auto& told = m_core_vector[ii];
          auto& tnew = newcorevec[ii];

          tnew = tensor<N, T>::create_template(*m_core_vector[0])
                     .map1(map1)
                     .map2(map2)
                     .name(m_name + "_work_core")
                     .build();

          copy(*told, *tnew).move_data(true).perform();
        }

        m_core_vector = std::move(newcorevec);
      }
      else {
        LOG.os<1>("Core tensors are compatible.\n");
      }

#else

      if (m_wrview.map1 != map1 || m_wrview.map2 != map2) {
        LOG.os<1>("Reordering core tensor.\n");

        auto new_work_tensor = tensor<N, T>::create_template(*m_work_tensor)
                                   .map1(map1)
                                   .map2(map2)
                                   .name(m_name + "_work_core")
                                   .build();

        copy(*m_work_tensor, *new_work_tensor).move_data(true).perform();

        // update
        m_wrview.dims = dims;
        m_wrview.map1 = map1;
        m_wrview.map2 = map2;
        m_wrview.nbatches = get_nbatches(dims);

        m_work_tensor.reset();
        m_work_tensor = new_work_tensor;
      }
      else {
        LOG.os<1>("Core tensor is compatible.\n");
      }

#endif
    }

    if (m_type == btype::direct) {
      LOG.os<1>("Creating direct work and read tensors.\n");

      m_work_tensor = tensor<N, T>::create()
                          .name(m_name + "_work_direct")
                          .set_pgrid(*m_spgrid_N)
                          .map1(map1)
                          .map2(map2)
                          .blk_sizes(m_blk_sizes)
                          .build();

      m_read_tensor = tensor<N, T>::create_template(*m_work_tensor)
                          .name(m_name + "_read_direct")
                          .build();

      // update
      m_wrview.dims = dims;
      m_wrview.map1 = map1;
      m_wrview.map2 = map2;
      m_wrview.nbatches = get_nbatches(dims);
    }

    if (m_type == btype::disk) {
      LOG.os<1>("Creating disk work and read tensors.\n");

      m_work_tensor = tensor<N, T>::create()
                          .name(m_name + "_work_disk")
                          .set_pgrid(*m_spgrid_N)
                          .map1(map1)
                          .map2(map2)
                          .blk_sizes(m_blk_sizes)
                          .build();

      m_read_tensor = tensor<N, T>::create_template(*m_work_tensor)
                          .name(m_name + "_read_disk")
                          .map1(m_wrview.map1)
                          .map2(m_wrview.map2)
                          .build();
    }

    if (m_type == btype::disk && !m_read_current_is_contiguous) {
      LOG.os<1>(
          "Reading will be non-contiguous. ",
          "Setting view of local block indices.\n");

      if (m_rdviewmap.find(dims) != m_rdviewmap.end()) {
        LOG.os<1>("View was already set previously. Using it.\n");
      }
      else {
        LOG.os<1>("View not yet computed. Doing it now...\n");
        m_rdviewmap[dims] = set_view(dims);
      }
    }

    // m_work_tensor->batched_contract_init();

    m_is_decompress_initialized = true;

    LOG.os<1>("Finished initializing decompression for ", m_name, "...\n");

    // std::cout << "NATCHED." << std::endl;
  }

  // if tensor_in nullptr, then gives back m_stensor
  void decompress(std::initializer_list<int> idx_list)
  {
    LOG.os<1>("Decompressing ", m_name, "...\n");

    if (!m_is_decompress_initialized) {
      throw std::runtime_error("Decompression not initialized.\n");
    }

    if (m_is_compress_initialized) {
      throw std::runtime_error(
          "Decompressing, but compression is initailized\n");
    }

    vec<int> idx = idx_list;

    switch (m_type) {
      case btype::disk:
        decompress_disk(idx);
        break;
      case btype::direct:
        decompress_direct(idx);
        break;
      case btype::core:
        decompress_core();
        break;
    }

    LOG.os<1>("Finished decompression for  ", m_name, "...\n");
  }

  void decompress_direct(vec<int> idx)
  {
    LOG.os<1>("Generating tensor entries.\n");

    auto b = get_blk_bounds(idx, m_read_current_dims);

    m_generator(m_read_tensor, b);
    m_read_tensor->filter(dbcsr::global::filter_eps);

    vec<vec<int>> copy_bounds = get_bounds(idx, m_wrview.dims);

    LOG.os<1>("Copying to work tensor.\n");

    copy(*m_read_tensor, *m_work_tensor)
        .bounds(copy_bounds)
        .move_data(true)
        .perform();
  }

  void decompress_core()
  {
    LOG.os<1>("Decompressing from core.\n");

#ifdef _CORE_VECTOR
    // std::cout << "Getting: " << flatten(idx, m_wrview.dims) << std::endl;
    m_work_tensor = m_core_vector[flatten(idx, m_wrview.dims)];
// dbcsr::print(*m_work_tensor);
#endif

    return;
  }

  void decompress_disk(vec<int> idx)
  {
    vec<int> dims = (m_read_current_is_contiguous) ?
        m_wrview.dims :
        m_rdviewmap[m_read_current_dims].dims;

    int ibatch = flatten(idx, dims);
    LOG.os<1>("Reading batch ", ibatch, '\n');

    m_work_tensor->clear();
    m_read_tensor->clear();

    // if reading is done in same way as writing
    if (m_read_current_is_contiguous) {
      LOG.os<1>("Same dimensions for writing and reading.\n");

      auto& nzeprocbatch = m_wrview.nzeprocbatch;
      auto& nblksprocbatch = m_wrview.nblksprocbatch;

      int nze = nzeprocbatch[ibatch][m_mpirank];
      int nblk = nblksprocbatch[ibatch][m_mpirank];

      // === Allocating blocks for tensor ===
      //// offsets

      int64_t blkoff = 0;

      for (int i = 0; i < ibatch; ++i) {
        blkoff += nblksprocbatch[i][m_mpirank];
      }

      // read idx and offset

      int64_t nblk_prev = 0;

      // global offset
      for (int i = 0; i != ibatch; ++i) {
        for (int m = 0; m != m_mpisize; ++m) {
          nblk_prev += nblksprocbatch[i][m];
        }
      }
      // local offset
      for (int m = 0; m != m_mpirank; ++m) {
        nblk_prev += nblksprocbatch[ibatch][m];
      }

      arrvec<int, N> newlocblkidx;
      vec<MPI_Offset> newlocblkoff;

      for (auto& l : newlocblkidx) l.resize(nblk);
      newlocblkoff.resize(nblk);

      MPI_File fh_idx;

      MPI_File_open(
          m_comm, m_wrview.file_name.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL,
          &fh_idx);

      MPI_Offset idx_offset =
          nblk_prev * (N * sizeof(int) + sizeof(MPI_Offset));

      for (auto& l : newlocblkidx) {
        MPI_File_read_at_all(
            fh_idx, idx_offset, l.data(), nblk, MPI_INT, MPI_STATUS_IGNORE);
        idx_offset += nblk * sizeof(int);
      }
      MPI_File_read_at_all(
          fh_idx, idx_offset, newlocblkoff.data(), nblk, MPI_OFFSET,
          MPI_STATUS_IGNORE);

      MPI_File_close(&fh_idx);

      if (LOG.global_plev() >= 10) {
        MPI_Barrier(m_comm);

        for (int i = 0; i != m_mpisize; ++i) {
          if (i == m_mpirank) {
            // std::cout << "NEW" << std::endl;
            for (auto a : newlocblkidx) {
              for (auto l : a) { std::cout << l << " "; }
              std::cout << std::endl;
            }
          }

          MPI_Barrier(m_comm);
        }
      }

      //// reserving
      m_read_tensor->reserve(newlocblkidx);

      for (auto& v : newlocblkidx) v.resize(0);

      // === Reading from file ===
      //// Allocating

      //// Offset
      MPI_Offset data_batch_offset = (nblk == 0) ? 0 : newlocblkoff[0];

      //// Opening File
      std::string fname = m_filename;

      MPI_File fh_data;

      MPI_File_open(
          m_comm, fname.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &fh_data);

      //// Reading
      long long int datasize;
      T* data = m_read_tensor->data(datasize);

      MPI_File_read_at_all(
          fh_data, data_batch_offset * sizeof(T), data, nze, MPI_DOUBLE,
          MPI_STATUS_IGNORE);

      MPI_File_close(&fh_data);

      LOG.os<1>("Copying to work tensor\n");

      auto copy_bounds = get_bounds(idx, m_read_current_dims);
      dbcsr::copy(*m_read_tensor, *m_work_tensor)
          .move_data(true)
          .bounds(copy_bounds)
          .perform();

      // needs special offsets
    }
    else {
      LOG.os<1>("Different dimension, but faster indices.\n");

      auto& rdview = m_rdviewmap[m_read_current_dims];

      auto& nzeprocbatch = rdview.nzeprocbatch;
      auto& nblksprocbatch = rdview.nblksprocbatch;

      int nze = nzeprocbatch[ibatch][m_mpirank];
      int nblk = nblksprocbatch[ibatch][m_mpirank];

      // read idx and offset
      int64_t blkoff = 0;

      for (int i = 0; i < ibatch; ++i) {
        blkoff += nblksprocbatch[i][m_mpirank];
      }

      int64_t nblk_prev = 0;

      // global offset
      for (int i = 0; i != ibatch; ++i) {
        for (int m = 0; m != m_mpisize; ++m) {
          nblk_prev += nblksprocbatch[i][m];
        }
      }
      // local offset
      for (int m = 0; m != m_mpirank; ++m) {
        nblk_prev += nblksprocbatch[ibatch][m];
      }

      arrvec<int, N> newlocblkidx;
      vec<MPI_Offset> newlocblkoff;

      for (auto& l : newlocblkidx) l.resize(nblk);
      newlocblkoff.resize(nblk);

      MPI_File fh_idx;

      MPI_File_open(
          m_comm, rdview.file_name.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL,
          &fh_idx);

      MPI_Offset idx_offset =
          nblk_prev * (N * sizeof(int) + sizeof(MPI_Offset));

      for (auto& l : newlocblkidx) {
        MPI_File_read_at_all(
            fh_idx, idx_offset, l.data(), nblk, MPI_INT, MPI_STATUS_IGNORE);
        idx_offset += nblk * sizeof(int);
      }
      MPI_File_read_at_all(
          fh_idx, idx_offset, newlocblkoff.data(), nblk, MPI_OFFSET,
          MPI_STATUS_IGNORE);

      MPI_File_close(&fh_idx);

      vec<MPI_Aint> blkoffsets(nblk);

      for (int i = 0; i != nblk; ++i) {
        blkoffsets[i] = newlocblkoff[i] * sizeof(T);
      }

      if (LOG.global_plev() >= 10) {
        MPI_Barrier(m_comm);

        LOG.os<>("LOC IDX AND OFFS\n");

        for (int i = 0; i != m_mpisize; ++i) {
          if (i == m_mpirank) {
            std::cout << "NEW" << std::endl;
            for (auto a : newlocblkidx) {
              for (auto l : a) { std::cout << l << " "; }
              std::cout << std::endl;
            }

            for (auto a : blkoffsets) { std::cout << a << " "; }
            std::cout << std::endl;
          }

          MPI_Barrier(m_comm);
        }
      }

      //// reserving
      T* buffer = new T[nze];
      // m_read_tensor->reserve(newlocblkidx);

      vec<int> blksizes(nblk);
      auto& tsizes = m_blk_sizes;

      LOG.os<1>("Computing block sizes.\n");

      for (int i = 0; i != nblk; ++i) {
        int size = 1;
        for (int n = 0; n != N; ++n) { size *= tsizes[n][newlocblkidx[n][i]]; }
        blksizes[i] = size;
      }

      LOG.os<1>("Reserving.\n");

      // for (auto& a : newlocblkidx) a.clear();

      // now adjust the file view
      // nze = m_read_tensor->num_nze();

      LOG.os<1>("Creating MPI TYPE.\n");

      if (LOG.global_plev() >= 10) {
        std::cout << "BLOCK SIZES + OFF: " << std::endl;
        for (int i = 0; i != nblk; ++i) {
          std::cout << blksizes[i] << " " << blkoffsets[i] << std::endl;
        }
      }

      MPI_Datatype MPI_HINDEXED;

      MPI_Type_create_hindexed(
          nblk, blksizes.data(), blkoffsets.data(), MPI_DOUBLE, &MPI_HINDEXED);
      MPI_Type_commit(&MPI_HINDEXED);

      std::string fname = m_filename;
      MPI_File fh_data;

      MPI_File_open(
          m_comm, fname.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &fh_data);

      LOG.os<1>("Setting file view.\n");

      MPI_File_set_view(
          fh_data, 0, MPI_DOUBLE, MPI_HINDEXED, "native", MPI_INFO_NULL);

      LOG.os<1>("Reading from file...\n");

      MPI_File_read_at_all(
          fh_data, 0, buffer, nze, MPI_DOUBLE, MPI_STATUS_IGNORE);

      MPI_File_close(&fh_data);

      LOG.os<1>("Done!!\n");

      MPI_Type_free(&MPI_HINDEXED);

      LOG.os<1>("Copying to work tensor\n");

      m_work_tensor->reserve(newlocblkidx);

      MPI_Offset buffer_offset = 0;

      auto map1 = m_work_tensor->map1_2d();
      auto map2 = m_work_tensor->map2_2d();

      auto rmap1 = rdview.map1;
      auto rmap2 = rdview.map2;

      std::array<int, N> idx, blksize;
      /*std::cout << "MAP: " << std::endl;
      for (auto m : map1) std::cout << m << " ";
      for (auto m : map2) std::cout << m << " ";
      std::cout << std::endl;*/

      for (int iblk = 0; iblk != nblk; ++iblk) {
        for (int i = 0; i != N; ++i) {
          idx[i] = newlocblkidx[i][iblk];
          blksize[i] = m_blk_sizes[i][idx[i]];
        }

        int rowsize = 1;
        int colsize = 1;

        for (auto m : m_wrview.map1) { rowsize *= m_blk_sizes[m][idx[m]]; }

        for (auto m : m_wrview.map2) { colsize *= m_blk_sizes[m][idx[m]]; }

        // get work tensor block
        bool found = false;
        auto blk_work = m_work_tensor->get_block(idx, blksize, found);

        if (!found) {
          throw std::runtime_error("Block not found...");
        }

        T* data = buffer + buffer_offset;

        blk_work.reshape_2d(data, m_wrview.map1, m_wrview.map2);

        m_work_tensor->put_block(idx, blk_work);

        buffer_offset += blk_work.ntot();
      }

      delete[] buffer;

    }  // end if-else
  }

  void decompress_finalize()
  {
    // m_work_tensor->batched_contract_finalize();
    if (m_type != btype::core) {
      m_work_tensor->clear();
      m_read_tensor->clear();
    }

#ifdef _CORE_VECTOR
    if (m_type == btype::core) {
      // m_work_tensor.reset();
    }
#endif

    m_is_decompress_initialized = false;
  }

  dbcsr::stensor<N, T> get_work_tensor()
  {
    return m_work_tensor;
  }

  int nbatches(int idim)
  {
    return m_nbatches_dim[idim];
  }

  vec<int> blk_bounds(int idim, int ibatch)
  {
    return m_blk_bounds[idim][ibatch];
  }

  vec<int> bounds(int idim, int ibatch)
  {
    return m_bounds[idim][ibatch];
  }

  vec<int> full_blk_bounds(int idim)
  {
    return m_full_blk_bounds[idim];
  }

  vec<int> full_bounds(int idim)
  {
    return m_full_bounds[idim];
  }

  double occupation()
  {
    std::array<int, N> full;
    for (int i = 0; i != N; ++i) {
      full[i] =
          std::accumulate(m_blk_sizes[i].begin(), m_blk_sizes[i].end(), 0);
    }
    int64_t tot = std::accumulate(
        full.begin(), full.end(), (int64_t)1, std::multiplies<int64_t>());

    return (double)m_nzetot / (double)tot;
  }

  void print_info()
  {
    std::array<int, N> full;
    for (int i = 0; i != N; ++i) {
      full[i] =
          std::accumulate(m_blk_sizes[i].begin(), m_blk_sizes[i].end(), 0);
    }

    int64_t tot = std::accumulate(
        full.begin(), full.end(), (int64_t)1, std::multiplies<int64_t>());

    LOG.setprecision(12);

    LOG.os<>(
        "Sparsity info of batch tensor ", m_name, '\n',
        "-- Block occupation: ", (double)m_nzetot / (double)tot, '\n',
        "-- Number of non-zero elements: ", (double)m_nzetot, '\n',
        "-- Total number of elements (dense): ", (double)tot, '\n');

    LOG.reset();
  }

  arrvec<int, N> blk_sizes()
  {
    return m_blk_sizes;
  }

  MPI_Comm comm()
  {
    return m_spgrid_N->comm();
  }

  shared_pgrid<N> spgrid()
  {
    return m_spgrid_N;
  }

  btype get_type()
  {
    return m_type;
  }

  std::array<int, N> batch_dims()
  {
    return m_nbatches_dim;
  }

  dbcsr::shared_tensor<N, T> get_template(
      std::string name, vec<int> map1, vec<int> map2)
  {
    auto out = dbcsr::tensor<N, T>::create()
                   .name(name)
                   .set_pgrid(*m_spgrid_N)
                   .blk_sizes(m_blk_sizes)
                   .map1(map1)
                   .map2(map2)
                   .build();

    return out;
  }

  std::shared_ptr<btensor<N, T>> get_access_duplicate()
  {
    auto out = std::make_shared<btensor<N, T>>(m_comm, LOG.global_plev());

    out->m_mpirank = m_mpirank;
    out->m_mpisize = m_mpisize;

    if (m_read_tensor) {
      out->m_read_tensor = dbcsr::tensor<3>::create_template(*m_read_tensor)
                               .name(m_read_tensor->name())
                               .build();
    }

    if (m_work_tensor) {
      out->m_work_tensor = m_work_tensor;  // for now
    }

    out->m_name = m_name;
    out->m_spgrid_N = m_spgrid_N;
    out->m_blk_sizes = m_blk_sizes;
    out->m_filename = m_filename;
    out->m_path = m_path;
    out->m_type = m_type;

    out->m_nblkloc = m_nblkloc;
    out->m_nblkloc_global = m_nblkloc_global;
    out->m_nzeloc = m_nzeloc;
    out->m_nblktot_global = m_nblktot_global;
    out->m_nblktot = m_nblktot;
    out->m_nzetot = m_nzetot;

    out->m_write_current_batch = m_write_current_batch;
    out->m_read_current_batch = m_read_current_batch;
    out->m_read_current_dims = m_read_current_dims;
    out->m_read_current_is_contiguous = m_read_current_is_contiguous;

    out->m_blk_bounds = m_blk_bounds;
    out->m_bounds = m_bounds;
    out->m_full_blk_bounds = m_full_blk_bounds;
    out->m_full_bounds = m_full_bounds;

    out->m_nbatches_dim = m_nbatches_dim;

    out->m_wrview = m_wrview;
    out->m_rdviewmap = m_rdviewmap;

    out->m_generator = m_generator;

    return out;
  }

};  // end class btensor

template <int N, typename T>
using sbtensor = std::shared_ptr<btensor<N, T>>;

}  // namespace dbcsr

#endif
