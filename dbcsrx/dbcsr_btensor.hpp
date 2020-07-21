#ifndef DBCSR_BTENSOR_HPP
#define DBCSR_BTENSOR_HPP

#include <dbcsr_tensor_ops.hpp>
#include "utils/mpi_time.h"

#include <map>
#include <mpi.h>
#include <cstdlib>
#include <memory>
#include <filesystem>

namespace dbcsr {
	
enum btype {
	core,
	disk,
	direct
};
	
template <int N, typename T, 
	typename = typename std::enable_if<(N >= 2 && N <= 4)>::type>
class btensor {
private:

	MPI_Comm m_comm;
	util::mpi_log LOG;
	int m_mpirank;
	int m_mpisize;
	
	dbcsr::stensor<N,T> m_stensor;
	// underlying shared tensor
	
	std::string m_filename;
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

	dbcsr::stensor<N,T> m_write_tensor;
	// tensor which we are reading from
	dbcsr::stensor<N,T> m_read_tensor;
	// tensor to which we are writing
	
	arrvec<vec<int>,N> m_blk_bounds;
	std::array<int,N> m_nbatches_dim;
	
	struct view {
		vec<int> dims;
		bool is_contiguous;
		int nbatches;
		arrvec<int,N> locblkidx;
		vec<MPI_Aint> locblkoff;
		vec<vec<int>> nblksprocbatch;
		vec<vec<int>> nzeprocbatch;
	};
	
	view m_wrview;
	std::map<vec<int>,view> m_rdviewmap;
	
	/* =========== FUNCTIONS ========== */
	
	vec<vec<int>> make_blk_bounds(std::vector<int> blksizes, 
		int nbatches) {
		
		int nblks = blksizes.size();
		int nele = std::accumulate(blksizes.begin(),blksizes.end(),0);
		
		if (nblks < nbatches) nbatches = nblks;
				
		double nele_per_batch = (double) nele / (double) nbatches;
		
		vec<vec<int>> out;
		int current_sum = 0;
		int ibatch = 1;
		int first_blk = 0;
		int last_blk = 0;
		
		for (int i = 0; i != nblks; ++i) {
			current_sum += blksizes[i];
			
			if (current_sum >= ibatch * nele_per_batch) {
				last_blk = i;
				vec<int> b = {first_blk,last_blk};
				out.push_back(b);
				first_blk = i+1;
				++ibatch;
			}
		}
		
		return out;
		
	}		
	
public:

	/* batchsize: given in MB */
	btensor(dbcsr::stensor<N,T>& stensor_in, int nbatches, 
		btype mytype, int print = -1) : 
		m_comm(stensor_in->comm()),
		m_filename(),
		m_mpirank(-1), m_mpisize(-1),
		LOG(stensor_in->comm(),print),
		m_type(mytype)
	{
		
		m_stensor = dbcsr::make_stensor<N,T>(
			typename dbcsr::tensor<N,T>::create_template(*stensor_in)
			.name(stensor_in->name() + "_batched")
		);
		
		MPI_Comm_rank(m_comm, &m_mpirank);
		MPI_Comm_size(m_comm, &m_mpisize);
		
		LOG.os<1>("Setting up batch tensor information.\n");
		
		std::string path = std::filesystem::current_path();
		path += "/batching/";
		
		m_filename = path + m_stensor->name() + ".dat";
		
		// divide dimensions
		
		auto blksizes = m_stensor->blk_sizes();
		
		for (int i = 0; i != N; ++i) {
			m_blk_bounds[i] = make_blk_bounds(blksizes[i],nbatches);
			m_nbatches_dim[i] = m_blk_bounds[i].size();
			// check if memory sufficient
			
			int nele = std::accumulate(blksizes[i].begin(),blksizes[i].end(),0);
			double nele_per_batch = (double) nele / (double) m_nbatches_dim[i];
			
			if (nele_per_batch * sizeof(T) >= 2 * 1e+9) {
				LOG.os<>(
					"WARNING: Batch sizes might be too large!\n",
					"Either increase number of batches, ",
					"or increase number of MPI processes.\n");
			}
			
		}
		
		LOG.os<1>("Batch bounds: \n");
		for (int i = 0; i != N; ++i) {
			LOG.os<1>("Dimension ", i, ":\n");
			for (auto b : m_blk_bounds[i]) {
				LOG.os<1>(b[0], " -> ", b[1], " ");
			} LOG.os<1>('\n');
		}
		
		if (m_type == disk) {
			create_file();
		}
		
		LOG.os<1>("Finished setting up batchtensor.\n");
		
		reset_var();
		
	}
	
	/* transforms a (flat) superindex into multiple tensor indices */ 
	/*std::vector<int> unroll_index(int64_t idx, std::vector<int> sizes) {
		
		int M = sizes.size();
		std::vector<int> out(M);
		
		if (M == 1) {
			out[0] = idx;
			return out;
		}
		
		int64_t dim = 1;
		
		for (int i = 0; i != M-1; ++i) {
			dim *= sizes[i];
		}
		
		out[M-1] = (int)(idx/dim);
		int64_t newidx = idx - out[M-1]*dim;
		
		if (M == 2) {
			out[0] = (int)newidx;
			return out;
		} else {
			std::vector<int> newsizes(M-1);
			std::copy(sizes.begin(),sizes.end()-1,newsizes.begin());
			auto newarr = unroll_index(newidx,newsizes);
			for (int i = 0; i != M-1; ++i) {
				out[i] = newarr[i];
			}
			return out;
		}
		
	}*/
	
	/* Create all necessary files. 
	 * !!! Deletes previous files and resets all variables !!! */
	void create_file() {
		
		delete_file();
		
		MPI_File fh;
		
		LOG.os<1>("Creating files for ", m_filename, '\n');
		
		int rc = MPI_File_open(m_comm,m_filename.c_str(),MPI_MODE_CREATE|MPI_MODE_WRONLY,
			MPI_INFO_NULL,&fh);
		
		MPI_File_close(&fh);
		
	}
	
	/* Deletes all files asscoiated with tensor. 
	 * !!! Resets variables !!! */
	void delete_file() {
		
		LOG.os<1>("Deleting files for ", m_filename, '\n');
		int rc = MPI_File_delete(m_filename.c_str(),MPI_INFO_NULL);
		
	}
	
	void reset_var() {
		
		m_nblkloc_global = 0;
		m_nblktot = 0;
		m_nblkloc = 0; 
		m_nzeloc = 0; 
		m_nblktot = 0; 
		m_nzetot = 0; 
		
		for (auto& v : m_wrview.locblkidx) {
			v.clear();
		}
		
		m_wrview.locblkoff.clear();		
		m_wrview.nblksprocbatch.clear();
		m_wrview.nzeprocbatch.clear();
	
		m_rdviewmap.clear();
	
	}	
	
	~btensor() { if (m_type == disk) delete_file(); }
	
	int flatten(vec<int>& idx, vec<int>& dims) {
		
		std::cout << "IDX/DIMS:" << std::endl;
		
		for (auto i : idx) {
			std::cout << i << " ";
		} std::cout << std::endl;
		
		for (auto i : dims) {
			std::cout << i << " ";
		} std::cout << std::endl;
		
		
		vec<int> bsizes(dims.size());
		for (int i = 0; i != bsizes.size(); ++i) {
			bsizes[i] = m_nbatches_dim[dims[i]];
		}
		
		int flat_idx = 0;
		
		for (int i = 0; i != bsizes.size(); ++i) {
			int off = 1;
			for (int n = bsizes.size() - 1; n > i; --n) {
				off *= bsizes[i];
			}
			flat_idx += idx[i] * off;
		}
		
		std::cout << "FLAT: " << flat_idx << std::endl;
		
		return flat_idx;
		
	}
	
	void compress_init(dbcsr::stensor<N,T>& tensor_in, 
		std::initializer_list<int> idx_list) 
	{
		
		LOG.os<1>("Initializing compression...\n");
		
		vec<int> idx = idx_list;
		m_write_tensor = tensor_in;
		
		m_wrview.nbatches = 1;
		m_wrview.dims = idx;
		m_wrview.is_contiguous = true;
		
		/* get total number of batches */
		for (int i = 0; i != idx.size(); ++i) {
			int idxi = idx[i];
			m_wrview.nbatches *= m_nbatches_dim[idxi];
		}
		
		LOG.os<1>("Total batches for writing: ", m_wrview.nbatches, '\n');
		
		reset_var();
		
	}
			
	
	/* ... */
	void compress(std::initializer_list<int> idx_list) {
	
		vec<int> idx = idx_list;
		
		switch(m_type) {
			case disk: compress_disk(idx);
			break;
			case core: compress_core(idx);
			break;
			case direct: compress_direct();
			break;
		}
		
	}
	
	void compress_direct() {
		m_write_tensor->clear();
		return;
	}
	
	void compress_core(vec<int> idx) {
		
		vec<vec<int>> b(N);
		auto& dims = m_wrview.dims;
		int ibatch = m_write_current_batch;
		
		for (int i = 0; i != idx.size(); ++i) {
			b[dims[i]] = this->bounds(dims[i])[idx[i]];
		}
		
		for (int i = 0; i != N; ++i) {
			auto iter = std::find(dims.begin(),dims.end(),i);
			b[i] = (iter == dims.end()) ? full_bounds(i) : b[i];
		}
    
		copy(*m_write_tensor, *m_stensor).bounds(b)
			.move_data(true).sum(true).perform();
					
	}
    
	void compress_disk(vec<int> idx) {
		
		auto dims = m_wrview.dims;
		int ibatch = flatten(idx,dims);
		int nbatches = m_wrview.nbatches;
				
		// writes the local blocks of batch ibatch to file
		// should only be called in order
		// blocks of a batch are stored as follows:
		
		// (batch1)                    (batch2)                .... 
		// (blks node1)(blks node2)....(blks node1)(blks node2)....
		
		// allocate data
		
		LOG.os<1>("Writing data of tensor ", m_filename, " to file.\n");
		LOG.os<1>("Batch ", ibatch, '\n');
		
		int nze = m_write_tensor->num_nze();
		int nblocks = m_write_tensor->num_blocks();
		
		LOG.os<1>("NZE/NBLOCKS: ", nze, "/", nblocks, '\n');
		
		auto& nblksprocbatch = m_wrview.nblksprocbatch;
		auto& nzeprocbatch = m_wrview.nzeprocbatch;
		
		nblksprocbatch.resize(nbatches);
		nzeprocbatch.resize(nbatches);
		
		nblksprocbatch[ibatch] = vec<int>(m_mpisize); 
		nzeprocbatch[ibatch] = vec<int>(m_mpisize);
				
		// communicate nzes + blks to other processes
		
		LOG.os<1>("Gathering nze and nblocks...\n");
		
		MPI_Allgather(&nze,1,MPI_INT,nzeprocbatch[ibatch].data(),1,MPI_INT,m_comm);
		MPI_Allgather(&nblocks,1,MPI_INT,nblksprocbatch[ibatch].data(),1,MPI_INT,m_comm);
		
		int64_t nblktotbatch = std::accumulate(nblksprocbatch[ibatch].begin(),
			nblksprocbatch[ibatch].end(),int64_t(0));
		
		int64_t nzetotbatch = std::accumulate(nzeprocbatch[ibatch].begin(),
			nzeprocbatch[ibatch].end(),int64_t(0));
		
		m_nblkloc += nblksprocbatch[ibatch][m_mpirank];
		m_nzeloc += nzeprocbatch[ibatch][m_mpirank];
		
		m_nblktot += nblktotbatch;
			
		m_nzetot += nzetotbatch;
		
		LOG(-1).os<1>("Local number of blocks: ", m_nblkloc, '\n');
		LOG.os<1>("Total number of blocks: ", nblktotbatch, '\n');
		LOG.os<1>("Total number of nze: ", nzetotbatch, '\n');
		
		// read blocks

		LOG.os<1>("Writing blocks...\n");

		dbcsr::iterator_t<N,T> iter(*m_write_tensor);
		
		iter.start();
		
		std::vector<MPI_Aint> blkoffbatch(nblocks); // block offsets in file for this batch
		arrvec<int,N> blkidxbatch; // block indices for this batch
		blkidxbatch.fill(vec<int>(nblocks));
		
		MPI_Aint offset = 0;
		int n = 0;
		
		int iblk = 0;
		
		while (iter.blocks_left()) {
			
			iter.next();
			
			auto& size = iter.size();
			auto& idx = iter.idx();
			
			for (int i = 0; i != N; ++i) {
				blkidxbatch[i][iblk] = idx[i];
			}
			
			int ntot = std::accumulate(size.begin(),size.end(),1,std::multiplies<int>());
			
			blkoffbatch[iblk++] = offset;
			
			offset += ntot;
			
		}
		
		iter.stop();
		
		// filenames
		
		std::string data_fname = m_filename;
		
		LOG.os<1>("Computing offsets...\n");
	
		// offsets
		MPI_Offset data_batch_offset = 0;
		
		//int64_t nblkprev = 0;
		int64_t ndataprev = 0;
		
		// global batch offset
		for (int i = 0; i != ibatch; ++i) {
				
			ndataprev += std::accumulate(nzeprocbatch[i].begin(),
				nzeprocbatch[i].end(),int64_t(0));
			
		}
		
		// local processor dependent offset
		for (int i = 0; i < m_mpirank; ++i) {
			ndataprev += nzeprocbatch[ibatch][i];
		}
		
		data_batch_offset = ndataprev;
		
		// add it to blkoffsets
		for (auto& off : blkoffbatch) {
			off += data_batch_offset;
		}
		
		// concatenate indices and offsets
		LOG.os<1>("Adding offsets to vector...\n");
		
		auto& locblkidx = m_wrview.locblkidx;
		auto& locblkoff = m_wrview.locblkoff;
		
		for (int i = 0; i != N; ++i) {
			locblkidx[i].insert(locblkidx[i].end(),
				blkidxbatch[i].begin(),blkidxbatch[i].end());
			blkidxbatch[i].clear();
		}
		
		locblkoff.insert(locblkoff.end(),
			blkoffbatch.begin(),blkoffbatch.end());
		blkoffbatch.clear();
	
		// write data to file 
		
		LOG.os<1>("Writing tensor data...\n");
		
		long long int datasize;
		T* data = m_write_tensor->data(datasize);
		
		MPI_File fh_data;
		
		MPI_File_open(m_comm,data_fname.c_str(),MPI_MODE_WRONLY,
			MPI_INFO_NULL,&fh_data);
		
		MPI_File_write_at_all(fh_data,data_batch_offset*sizeof(T),data,
			nze,MPI_DOUBLE,MPI_STATUS_IGNORE);
			
		MPI_File_close(&fh_data);
		
		if (LOG.global_plev() >= 10) {
			std::cout << "LOC BLK IDX AND OFFSET." << std::endl;
			for (int i = 0; i != locblkidx[0].size(); ++i) {
				for (int j = 0; j != N; ++j) {
					std::cout << locblkidx[j][i] << " ";
				}
				std::cout << locblkoff[i] << std::endl;
			}
		}
		
		m_write_tensor->clear();
		
		LOG.os<1>("Done with batch ", ibatch, '\n');
		
	}
	
	void compress_finalize() {
		
		LOG.os<1>("Finalizing compression...\n");
		
		m_write_tensor.reset();
		
	}
	
	view set_view(vec<int> dims) {
		
		view rview;
		
		rview.is_contiguous = false;
		rview.dims = dims;
		
		// check order of indices
		vec<int> order;
		order.insert(order.end(),dims.begin(),dims.end());
		
		for (int i = 0; i != N - dims.size(); ++i) {
			auto iter = std::find(order.begin(),order.end(),i);
			if (iter == order.end()) order.push_back(i);
		}
		
		// reorder indices
		for (auto i : order) {
			LOG.os<1>(i, " ");
		} LOG.os<1>('\n');
		
		auto idxspeed = m_stensor->idx_speed();
		
		vec<int> order_rev(3);
		
		for (int i = 0; i != N; ++i) {
			auto iter = std::find(order.begin(),order.end(),i);
			order_rev[i] = iter - order.begin();
		}
		
		// reorder reverse indices
		for (auto i : order_rev) {
			LOG.os<1>(i, " ");
		} LOG.os<1>('\n');
		
		
		// compute super indices
		
		auto& nblksprocbatch = rview.nblksprocbatch;
		auto& nzeprocbatch = rview.nzeprocbatch;
		auto& rlocblkidx = rview.locblkidx;
		auto& rlocblkoff = rview.locblkoff;
		auto& wlocblkidx = m_wrview.locblkidx;
		auto& wlocblkoff = m_wrview.locblkoff;
		
		vec<size_t> perm(wlocblkidx[0].size()), superidx(wlocblkidx[0].size());
		
		std::iota(perm.begin(),perm.end(),(size_t)0);
		std::iota(superidx.begin(),superidx.end(),(size_t)0);
		
		// get nblk_per_batch for all dims
		vec<double> nele_per_batch(N);
		
		auto blksizes = m_stensor->blk_sizes();
		
		for (int i = 0; i != N; ++i) {
			int nele = std::accumulate(blksizes[i].begin(),
				blksizes[i].end(),0);
			int nbatch = m_nbatches_dim[i];
			nele_per_batch[i] = (double) nele / (double) nbatch;
			std::cout << "NELE: " << nele_per_batch[i] << std::endl;
		}	
		
		if (N == 3) {
			
			const size_t dim0 = order[0];
			const size_t dim1 = order[1];
			const size_t dim2 = order[2];
			
			const size_t bsize0 = (dims.size() > 0) ? m_nbatches_dim[dim0] : 1;
			const size_t bsize1 = (dims.size() > 1) ? m_nbatches_dim[dim1] : 1;
			const size_t bsize2 = (dims.size() > 2) ? m_nbatches_dim[dim2] : 1;
			
			const size_t batchsize = bsize0 * bsize1 * bsize2;
			
			rview.nbatches = batchsize;
			
			auto sizes = m_stensor->blk_sizes();
			auto full = m_stensor->nblks_total();
			auto off = m_stensor->blk_offsets();
			
			size_t nblks = std::accumulate(full.begin(),full.end(),
				1, std::multiplies<size_t>());
				
			const size_t blksize1 = full[idxspeed[1]];
			const size_t blksize2 = full[idxspeed[0]];
			
			nblksprocbatch = vec<vec<int>>(batchsize,vec<int>(m_mpisize));
			nzeprocbatch = vec<vec<int>>(batchsize,vec<int>(m_mpisize));
			
			for (size_t i = 0; i != superidx.size(); ++i) {	
									
					const size_t i0 = wlocblkidx[idxspeed[2]][i];
					const size_t i1 = wlocblkidx[idxspeed[1]][i];
					const size_t i2 = wlocblkidx[idxspeed[0]][i];
					
					const size_t b0 = std::floor(
						(double)off[dim0][wlocblkidx[dim0][i]]
						/ nele_per_batch[dim0]);
					const size_t b1 = (dims.size() > 1) ? 
						std::floor(
						(double)off[dim1][wlocblkidx[dim1][i]] 
						/ nele_per_batch[dim1]) : 0;
					const size_t b2 = (dims.size() > 2) ?
						std::floor(
						(double)off[dim2][wlocblkidx[dim2][i]] 
						/ nele_per_batch[dim2]) : 0;
						
					const size_t batch_idx = b0 * bsize1 * bsize2 + b1 * bsize2 + b2;
					int size = sizes[idxspeed[2]][i0] 
						* sizes[idxspeed[1]][i1]
						* sizes[idxspeed[0]][i2];
					
					nblksprocbatch[batch_idx][m_mpirank]++;
					nzeprocbatch[batch_idx][m_mpirank]++;
									
					superidx[i] = batch_idx * nblks
					 + i0 * blksize2 * blksize1
					 + i1 * blksize2
					 + i2;
					 
					 std::cout << "INTEGER: " << superidx[i] << " "
						<< " BATCHES " << b0 << " " << b1 << " " << b2
						<< " IDX " << i0 << " " << i1 << " " << i2 << std::endl;
					 
			}
				 
			// communicate
			LOG.os<1>("Gathering nze and nblocks...\n");
		
			for (int i = 0; i != batchsize; ++i) {
				
				int nblk = nblksprocbatch[i][m_mpirank];
				int nze = nzeprocbatch[i][m_mpirank];
				
				MPI_Allgather(&nze,1,MPI_INT,nzeprocbatch[i].data(),1,MPI_INT,m_comm);
				MPI_Allgather(&nblk,1,MPI_INT,nblksprocbatch[i].data(),1,MPI_INT,m_comm);
				
			}
				 
		}
		
		std::sort(perm.begin(),perm.end(),
			[&](size_t i0, size_t i1) {
				return superidx[i0] < superidx[i1];
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
				for (auto i : a) {
					LOG.os<1>(i, " ");
				} LOG.os<1>('\n');
			}
			for (auto a : rlocblkoff) {
				LOG.os<1>(a, " ");
			} LOG.os<1>('\n');
		}
		
		return rview;
			
	}
		
	
	void decompress_init(dbcsr::stensor<N,T>& t_in, 
		std::initializer_list<int> dims_list) 
	{
		
		vec<int> dims = dims_list;
		
		LOG.os<1>("Initializing decompression...\n");
		
		m_read_current_is_contiguous = (dims == m_wrview.dims) ? true : false;
		m_read_tensor = t_in;
		m_read_current_dims = dims;
		
		if (m_type != disk) return;
		
		if (!m_read_current_is_contiguous) {
			
			LOG.os<1>("Reading will be non-contiguous. ",
				"Setting view of local block indices.\n");
				
			if (m_rdviewmap.find(dims) != m_rdviewmap.end()) {
				LOG.os<1>("View was already set previously. Using it.\n");
			} else {
				LOG.os<1>("View not yet computed. Doing it now...\n");
				m_rdviewmap[dims] = set_view(dims);	
			}
			
		}
		
	}
	
	void decompress(std::initializer_list<int> idx_list) {
		
		vec<int> idx = idx_list;
		
		switch(m_type) {
			case disk : decompress_disk(idx);
			break;
			//case direct : decompress_direct(idx);
			//break;
			case core : decompress_core(idx);
		}
		
	}
	
	
	void decompress_core(vec<int> idx) {
		
		vec<vec<int>> b(N);
		auto& dims = m_read_current_dims;	
		
		for (int i = 0; i != idx.size(); ++i) {
			b[dims[i]] = this->bounds(dims[i])[idx[i]];
		}
		
		for (int i = 0; i != N; ++i) {
			auto iter = std::find(dims.begin(),dims.end(),i);
			b[i] = (iter == dims.end()) ? full_bounds(i) : b[i];
		}
    
		copy(*m_stensor, *m_read_tensor).bounds(b)
			.perform();
				
	}
				
		
	/* ... */
	void decompress_disk(vec<int> idx) {
		
		vec<int> dims = (m_read_current_is_contiguous) ?
			m_wrview.dims : m_rdviewmap[m_read_current_dims].dims;
		
		int ibatch = flatten(idx, dims);
		LOG.os<1>("Reading batch ", ibatch, '\n');
		
		// if reading is done in same way as writing
		if (m_read_current_is_contiguous) {
			
			LOG.os<1>("Same dimensions for writing and reading.\n");
			
			auto& nzeprocbatch = m_wrview.nzeprocbatch;
			auto& nblksprocbatch = m_wrview.nblksprocbatch;
			auto& locblkidx = m_wrview.locblkidx;
			auto& locblkoff = m_wrview.locblkoff;
		
			int nze = nzeprocbatch[ibatch][m_mpirank];
			int nblk = nblksprocbatch[ibatch][m_mpirank];
			
			int64_t nblktotbatch = std::accumulate(nblksprocbatch[ibatch].begin(),
				nblksprocbatch[ibatch].end(),int64_t(0));
		
			int64_t nzetotbatch = std::accumulate(nzeprocbatch[ibatch].begin(),
				nzeprocbatch[ibatch].end(),int64_t(0));
			
			// === Allocating blocks for tensor ===
			//// offsets
			
			int64_t blkoff = 0;
			
			for (int i = 0; i < ibatch; ++i) {
				blkoff += nblksprocbatch[i][m_mpirank];
			}
				
			arrvec<int,N> rlocblkidx;
			
			LOG(-1).os<1>("Offset: ", blkoff, '\n');
			LOG(-1).os<1>("NBLKTOTBATCH: ", nblktotbatch, '\n');
			//// setting
			for (int i = 0; i != N; ++i) {
				rlocblkidx[i].insert(rlocblkidx[i].end(),
					locblkidx[i].begin() + blkoff,
					locblkidx[i].begin() + blkoff + nblk);
			}
			
			if (LOG.global_plev() >= 10) {
			
				MPI_Barrier(m_comm);
				
				for (int i = 0; i != m_mpisize; ++i) {
				
					if (i == m_mpirank) {
					
						for (auto a : rlocblkidx) {
							for (auto l : a) {
								std::cout << l << " ";
							} std::cout << std::endl;
						}
						
					}
				
				MPI_Barrier(m_comm);
				
				}
				
			}
			
			//// reserving
			m_read_tensor->reserve(rlocblkidx);
			
			for (auto& v : rlocblkidx) v.resize(0);
			
			// === Reading from file ===
			//// Allocating
			
			//// Offset
			MPI_Offset data_batch_offset = locblkoff[blkoff];
			
			//// Opening File
			std::string fname = m_filename;
			
			MPI_File fh_data;
			
			MPI_File_open(m_comm,fname.c_str(),MPI_MODE_RDONLY,
				MPI_INFO_NULL,&fh_data);
		
			//// Reading
			long long int datasize;
			T* data = m_read_tensor->data(datasize);
			
			MPI_File_read_at(fh_data,data_batch_offset*sizeof(T),data,
				nze,MPI_DOUBLE,MPI_STATUS_IGNORE);
			
			MPI_File_close(&fh_data);
		
		// needs special offsets
		} else {
			
			LOG.os<1>("Different dimension.\n");
			
			auto& rdview = m_rdviewmap[m_read_current_dims];
			
			auto& nzeprocbatch = rdview.nzeprocbatch;
			auto& nblksprocbatch = rdview.nblksprocbatch;
			auto& locblkidx = rdview.locblkidx;
			auto& locblkoff = rdview.locblkoff;
		
			int nze = nzeprocbatch[ibatch][m_mpirank];
			int nblk = nblksprocbatch[ibatch][m_mpirank];
			
			int64_t nblktotbatch = std::accumulate(nblksprocbatch[ibatch].begin(),
				nblksprocbatch[ibatch].end(),int64_t(0));
		
			int64_t nzetotbatch = std::accumulate(nzeprocbatch[ibatch].begin(),
				nzeprocbatch[ibatch].end(),int64_t(0));
			
			// === Allocating blocks for tensor ===
			//// offsets
			
			int64_t blkoff = 0;
			
			for (int i = 0; i < ibatch; ++i) {
				blkoff += nblksprocbatch[i][m_mpirank];
			}
				
			arrvec<int,N> rlocblkidx;
			vec<MPI_Aint> blkoffsets(nblk);
			
			LOG(-1).os<1>("Offset: ", blkoff, '\n');
			LOG(-1).os<1>("NBLKTOTBATCH: ", nblktotbatch, '\n');
			//// setting
			for (int i = 0; i != N; ++i) {
				rlocblkidx[i].insert(rlocblkidx[i].end(),
					locblkidx[i].begin() + blkoff,
					locblkidx[i].begin() + blkoff + nblk);
			}
			
			for (size_t i = 0; i != nblk; ++i) {
				blkoffsets[i] = locblkoff[i + blkoff] * sizeof(T);
			}
			
			if (LOG.global_plev() >= 10) {
			
				MPI_Barrier(m_comm);
				
				LOG.os<>("LOC IDX AND OFFS\n");
				
				for (int i = 0; i != m_mpisize; ++i) {
				
					if (i == m_mpirank) {
						
						for (auto a : rlocblkidx) {
							for (auto l : a) {
								std::cout << l << " ";
							} std::cout << std::endl;
						}
						
						for (auto a : blkoffsets) {
							std::cout << a << " ";
						} std::cout << std::endl;
						
						
					}
				
				MPI_Barrier(m_comm);
				
				}
				
			}
			
			//// reserving
			m_read_tensor->reserve(rlocblkidx);
			
			vec<int> blksizes(nblk);
			auto tsizes = m_stensor->blk_sizes();
			
			LOG.os<1>("Computing block sizes.\n");
			
			for (int i = 0; i != nblk; ++i) {
				int size = 1;
				for (int n = 0; n != N; ++n) {
					size *= tsizes[n][rlocblkidx[n][i]];
				}
				blksizes[i] = size;
			}
			
			LOG.os<1>("Reserving.\n");
						
			for (auto& a : rlocblkidx) a.clear();
			
			// now adjust the file view
			nze = m_read_tensor->num_nze();
			
			LOG.os<1>("Creating MPI TYPE.\n");
			
			if (LOG.global_plev() >= 10) {
			
				std::cout << "BLOCK SIZES + OFF: " << std::endl;
				for (int i = 0; i != nblk; ++i) {
					std::cout << blksizes[i] << " " << blkoffsets[i] << std::endl;
				}
				
			}
			
			MPI_Datatype MPI_HINDEXED;
			
			MPI_Type_create_hindexed(nblk,blksizes.data(),blkoffsets.data(),MPI_DOUBLE,&MPI_HINDEXED);
			MPI_Type_commit(&MPI_HINDEXED);
			
			std::string fname = m_filename;
			MPI_File fh_data;
			
			MPI_File_open(m_comm,fname.c_str(),MPI_MODE_RDONLY,MPI_INFO_NULL,&fh_data);
			
			LOG.os<1>("Setting file view.\n");
			
			MPI_File_set_view(fh_data, 0, MPI_DOUBLE, MPI_HINDEXED, "native", MPI_INFO_NULL);
			
			long long int datasize;
			T* data = m_read_tensor->data(datasize);
			
			LOG.os<1>("Reading from file...\n");
			
			MPI_File_read_at(fh_data,0,data,nze,MPI_DOUBLE,MPI_STATUS_IGNORE);
			
			MPI_File_close(&fh_data);
			
			LOG.os<1>("Done!!\n");
			
		} // end if*/

	}
	
	void decompress_finalize() {
		m_read_tensor.reset();
	}
		
	//dbcsr::stensor<N,T> get_stensor() { return m_stensor; }
		
	//int nbatches() { return m_nbatches; }
	
	/* returns bounds for batch ibatch and tensor dimension idim
	 * for use in dbcsr_copy, dbcsr_contract etc. ... */
	/*vec<int> bounds(int ibatch, int idim) { 
		
		vec<int> bblk = this->bounds_blk(ibatch,idim);
		vec<int> out(2);
		
		out[0] = m_blkoffsets[idim][bblk[0]];
		out[1] = m_blkoffsets[idim][bblk[1]] + m_blksizes[idim][bblk[1]] - 1;

		return out;
			
	}*/
	
	/* returns the BLOCK bounds */
	/*vec<int> bounds_blk(int ibatch, int idim) { 
		
		vec<int> out(2);
		
		if (m_onebatch) {
			out[0] = 0;
			out[1] = m_blkstot[idim] - 1;
			return out;
		}
		
		auto iter = std::find(m_current_batchdim.begin(),
			m_current_batchdim.end(),idim);
			
		if (iter != m_current_batchdim.end()) {
			int pos = iter - m_current_batchdim.begin();
			out[0] = m_boundsblk[ibatch][pos][0];
			out[1] = m_boundsblk[ibatch][pos][1];
		} else {
			out[0] = 0;
			out[1] = m_blkstot[idim] - 1;
		}
		
		return out;
			
	}
	*/
	
	int nbatches_dim(int idim) {
		return m_nbatches_dim[idim];
	}
	
	vec<vec<int>> blk_bounds(int idim) {
		return m_blk_bounds[idim];
	}
	
	vec<vec<int>> bounds(int idim) {
		auto out = this->blk_bounds(idim);
		auto blkoffs = m_stensor->blk_offsets()[idim];
		for (int i = 0; i != out.size(); ++i) {
			out[i][0] = blkoffs[out[i][0]];
			out[i][1] = blkoffs[out[i][1]];
		}
		return out;
	}
	
	vec<int> full_blk_bounds(int idim) {
		auto full = m_stensor->nblks_total();
		vec<int> out = {0,full[idim]-1};
		return out;
	}
	
	vec<int> full_bounds(int idim) {
		auto full = m_stensor->nfull_total();
		vec<int> out = {0,full[idim]-1};
		return out;
	}

	
}; //end class btensor

template <int N, typename T>
using sbtensor = std::shared_ptr<btensor<N,T>>;
	
} //end namespace

#endif
