#include "adc/adcmod.h"

namespace adc {

/*
void get_diag_4(dbcsr::tensor<4>& t4, dbcsr::tensor<2>& t2, vec<int>& isizes, 
	vec<int>& asizes, int order) {
	
	// if order = 0 -> iiaa
	// else -> iaia
	
	int myrank = -1;
	MPI_Comm comm = t4.comm();
	
	MPI_Comm_rank(comm, &myrank);
	
	for (int iblk = 0; iblk != isizes.size(); ++iblk) {
			for (int ablk = 0; ablk != asizes.size(); ++ablk) {
			
				int proc_2 = -1;
				int proc_4 = -1;
				
				int isize = isizes[iblk];
				int asize = asizes[ablk];
				
				dbcsr::idx2 idx_2 = {iblk,ablk};
				dbcsr::idx4 idx_4 = (order == 0) ? dbcsr::idx4{iblk,iblk,ablk,ablk} : dbcsr::idx4{iblk,ablk,iblk,ablk};
				
				proc_4 = t4.proc(idx_4);
				proc_2 = t2.proc(idx_2);
				
				std::cout << "IA " << iblk << " " << ablk << std::endl;
				
				dbcsr::idx2 size_2 = {isize,asize};
				dbcsr::block<2> blk2(size_2);
				
				bool found4 = false;
				
				// get block from t4
				if (myrank == proc_4) {
					
					auto blk_size = (order == 0) ? arr<int,4>{isize,isize,asize,asize} 
						: arr<int,4>{isize,asize,isize,asize};
					auto blk4 = t4.get_block(idx_4, blk_size, found4);
					
					//extract diagonal from block
					if (found4) {
						if (order == 0) {
							std::cout << "HERE" << std::endl;
							for (int i = 0; i != isize; ++i) {
								for (int a = 0; a != asize; ++a) {		
									blk2(i,a) = blk4(i,i,a,a);
							}}
						} else {
							for (int i = 0; i != isize; ++i) {
								for (int a = 0; a != asize; ++a) {		
									blk2(i,a) = blk4(i,a,i,a);
							}}
						}
					}
					
				}
				
				MPI_Bcast(&found4,1,MPI_C_BOOL,proc_4,comm);
					
				if (found4) {
					
					// Send block to correct processor
					if (myrank == proc_4 && proc_2 != proc_4) {
						MPI_Send(blk2.data(),blk2.ntot(),MPI_DOUBLE,proc_2,1,comm);
					} else if (myrank == proc_2 && proc_2 != proc_4) {
						MPI_Recv(blk2.data(),blk2.ntot(),MPI_DOUBLE,proc_4,1,comm, MPI_STATUS_IGNORE);
					}
					
					if (myrank == proc_2) {
						std::cout << "PUT" << std::endl; 
						t2.put_block(idx_2, blk2);
					}
				}
				
				MPI_Barrier(comm);
					
			} // end ablk
		} // end iblk
		
}

void adcmod::mo_compute_diag() {
		
		int myproc = -1;
		MPI_Comm_rank(m_comm,&myproc);
		
		// Make diag_ia_1 = (ii|aa)
		dbcsr::pgrid<4> grid4(m_comm);
		dbcsr::pgrid<2> grid2(m_comm);
		
		arrvec<int,4> blksizes_iiaa = {m_dims.o,m_dims.o,m_dims.v,m_dims.v};
		
		dbcsr::tensor<4> d_iiaa = dbcsr::tensor<4>::create().name("d_iiaa").ngrid(grid4)
			.map1({0,1}).map2({2,3}).blk_sizes(blksizes_iiaa);
			
		// Reserve diagonal blocks
		auto nblkstot = d_iiaa.nblks_total();
		arrvec<int,4> res_iiaa;
		arrvec<int,4> res_iaia;
		
		for (int I = 0; I != nblkstot[0]; ++I) {
		 for (int A = 0; A != nblkstot[2]; ++A) {
			 int proc = -1;
			 dbcsr::idx4 idx = {I,I,A,A};
			 proc = d_iiaa.proc(idx);
			 
			 if (proc == myproc) {
				 res_iiaa[0].push_back(I);
				 res_iaia[0].push_back(I);
				 
				 res_iiaa[1].push_back(I);
				 res_iaia[1].push_back(A);
				 
				 res_iiaa[2].push_back(A);
				 res_iaia[2].push_back(I);
				 
				 res_iiaa[3].push_back(A);
				 res_iaia[3].push_back(A);
			 }
			 
		 }
		}
		
		d_iiaa.reserve(res_iiaa);
		
		// now contract
		
		//dbcsr::einsum<3,3,4>({.x = "Xij, Xab -> ijab", .t1 = *m_mo.b_xoo, .t2 = *m_mo.b_xvv, 
		//	.t3 = d_iiaa, .retain_sparsity = true});
		dbcsr::contract(*m_mo.b_xoo, *m_mo.b_xvv, d_iiaa).retain_sparsity(true).perform("Xij, Xab -> ijab");	
			
		dbcsr::print(d_iiaa);
			
		// extract the diagonal
		
		arrvec<int,2> blksizes_2 = {m_dims.o, m_dims.v};
		
		dbcsr::tensor<2> diag_ia_1 = dbcsr::tensor<2>::create().name("diag_ia_1").ngrid(grid2) 
			.map1({0}).map2({1}).blk_sizes(blksizes_2);
			
		diag_ia_1.reserve_all();
		
		get_diag_4(d_iiaa,diag_ia_1,m_dims.o,m_dims.v,0);
		
		d_iiaa.destroy();
			
		dbcsr::print(diag_ia_1);
		
		// now make (ia|ia)
		
		arrvec<int,4> blksizes_iaia = {m_dims.o, m_dims.v, m_dims.o, m_dims.v};
		dbcsr::tensor<4> d_iaia = dbcsr::tensor<4>::create().name("d_iaia").ngrid(grid4)
			.map1({0,1}).map2({2,3}).blk_sizes(blksizes_iaia);
		
		d_iaia.reserve(res_iaia);
		
		//dbcsr::einsum<3,3,4>({.x = "Xia, Xjb -> iajb", .t1 = *m_mo.b_xov, .t2 = *m_mo.b_xov, .t3 = d_iaia, .retain_sparsity = true});
		dbcsr::contract(*m_mo.b_xov, *m_mo.b_xov, d_iaia).retain_sparsity(true).perform("Xia, Xjb -> iajb");
		
		dbcsr::print(d_iaia);
		
		// now extract them
		
		dbcsr::tensor<2> diag_ia_2 = dbcsr::tensor<2>::create_template()
			.tensor_in(diag_ia_1).name("diag_ia_2");
		diag_ia_2.reserve_all();
		
		get_diag_4(d_iaia,diag_ia_2,m_dims.o,m_dims.v,1);
				
		d_iaia.destroy();
		dbcsr::print(diag_ia_2);	
		
		// form eps_ia
		
		//dbcsr::tensor<2> eps_ia({.tensor_in = diag_ia_1, .name = "eps_ia"});
		dbcsr::tensor<2> eps_ia = dbcsr::tensor<2>::create_template()
			.tensor_in(diag_ia_1).name("eps_ia");
		
		eps_ia.reserve_all();
		
		dbcsr::iterator_t iter2(eps_ia);
		iter2.start();
		
		auto epso = m_hfwfn->eps_occ_A();
		auto epsv = m_hfwfn->eps_vir_A();
		
		while (iter2.blocks_left()) {
			
			iter2.next();
			
			auto& idx_2 = iter2.idx();
			auto& blksize = iter2.size();
			auto& off = iter2.offset();
		
			dbcsr::block<2> blk2(blksize);
			
			for (int io = 0; io != blksize[0]; ++io) {
				for (int iv = 0; iv != blksize[1]; ++iv) {
					blk2(io,iv) = - epso->at(io + off[0]) + epsv->at(iv + off[1]);
				}
			}
			
			eps_ia.put_block(idx_2,blk2);
			
		}
		
		dbcsr::print(eps_ia);
		
		// Form d_ia = eps_ia - 2 * (ii|aa) - (ia|ia)
		
		m_mo.d_ov = dbcsr::make_stensor<2>(
			dbcsr::tensor<2>::create_template().tensor_in(eps_ia).name("d_ov"));

		diag_ia_1.scale(-1.0);
		//diag_ia_2.scale(2.0);
		
		//dbcsr::copy<2>({.t_in = eps_ia, .t_out = *m_mo.d_ov, .move_data = true});
		//dbcsr::copy<2>({.t_in = diag_ia_1, .t_out = *m_mo.d_ov, .sum = true, .move_data = true});
		//dbcsr::copy<2>({.t_in = diag_ia_2, .t_out = *m_mo.d_ov, .sum = true, .move_data = true});
		
		dbcsr::copy(eps_ia, *m_mo.d_ov).move_data(true).perform();
		dbcsr::copy(diag_ia_1, *m_mo.d_ov).sum(true).move_data(true).perform();
		dbcsr::copy(diag_ia_2, *m_mo.d_ov).sum(true).move_data(true).perform();
		
		dbcsr::print(*m_mo.d_ov);
		
		diag_ia_1.destroy();
		diag_ia_2.destroy();
		eps_ia.destroy();
		
}*/

void adcmod::compute_diag() {
	
	// only zero order for now
	
	LOG.os<>("Computing zeroth order diagonal.\n");
	
	auto epso = m_hfwfn->eps_occ_A();
	auto epsv = m_hfwfn->eps_vir_A();
	
	auto o = m_hfwfn->mol()->dims().oa();
	auto v = m_hfwfn->mol()->dims().va();
	
	m_d_ov = dbcsr::create<double>()
		.name("diag_ov")
		.set_world(m_world)
		.row_blk_sizes(o)
		.col_blk_sizes(v)
		.matrix_type(dbcsr::type::no_symmetry)
		.get();
		
	m_d_ov->reserve_all();
	
	dbcsr::iterator<double> iter(*m_d_ov);
	
	iter.start();
	
	while (iter.blocks_left()) {
		
		iter.next_block();
		
		int roff = iter.row_offset();
		int coff = iter.col_offset();
		
		int rsize = iter.row_size();
		int csize = iter.col_size();
		
		for (int i = 0; i != rsize; ++i) {
			for (int j = 0; j != csize; ++j) {
				iter(i,j) = - epso->at(i + roff) 
					+ epsv->at(j + coff);
			}
		}
		
	}
	
	iter.stop();
	
	if (LOG.global_plev() >= 2) dbcsr::print(*m_d_ov);
	
	LOG.os<>("Done with diagonal.\n");
	
}

} // end namespace
		