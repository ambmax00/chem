#include "math/solvers/hermitian_eigen_solver.h"
#include <dbcsr_matrix_ops.hpp>

#ifdef USE_SCALAPACK
#include "extern/scalapack.h"
#else
#include <Eigen/Eigenvalues>
#endif

#include <dbcsr_conversions.hpp>
#include <cmath>

namespace math {
	
#ifdef USE_SCALAPACK

void hermitian_eigen_solver::compute(int scalapack_blksize) {
	
	int lwork;
	
	int n = m_mat_in->nfullrows_total();
	int nb = scalapack_blksize;
	int nprow = m_world.dims()[0];
	int npcol = m_world.dims()[1];
	
	LOG.os<>("Running SCALAPACK pdsyev calculation\n");
	
	if (m_jobz == 'N') {
		lwork = 5*n + nb * ((n - 1)/nprow + nb) + 1;
	} else {
		int A = (n/(nb*nprow)) + (n/(nb*npcol));
		lwork = 5*n + std::max(2*n,(3+A)*nb*nb) + nb*((n-1)/(nb*nprow*npcol)+1)*n + 1;
	}
	
	LOG.os<>("-- Allocating work space of size ", lwork, '\n');
	double* work = new double[lwork];
	
	// convert array
	
	LOG.os<>("-- Converting input matrix to SCALAPACK format.\n");
		
	int ori_proc = m_mat_in->proc(0,0);
	int ori_coord[2];
	
	if (m_mat_in->get_world().rank() == ori_proc) {
		ori_coord[0] = scalapack::global_grid.myprow();
		ori_coord[1] = scalapack::global_grid.mypcol();
	}
	
	MPI_Bcast(&ori_coord[0],2,MPI_INT,ori_proc,m_mat_in->get_world().comm());
		
	scalapack::distmat<double> sca_mat_in = dbcsr::matrix_to_scalapack(*m_mat_in, 
		m_mat_in->name() + "_scalapack", nb, nb, ori_coord[0], ori_coord[1]);
	
	std::optional<scalapack::distmat<double>> sca_eigvec_opt;
	
	if (m_jobz == 'V') {
		sca_eigvec_opt.emplace(n, n, nb, nb, ori_coord[0], ori_coord[1]);
	} else {
		sca_eigvec_opt = std::nullopt;
	}
	
	m_eigval.resize(n);
	
	double* a_ptr = sca_mat_in.data();
	double* z_ptr = (sca_eigvec_opt) ? sca_eigvec_opt->data() : nullptr;
	auto sca_desc = sca_mat_in.desc();
	int info;
	
	LOG.os<>("-- Starting pdsyev...\n");
	c_pdsyev(m_jobz,'U',n,a_ptr,0,0,sca_desc.data(),m_eigval.data(),z_ptr,
		0,0,sca_desc.data(),work,lwork,&info);
	LOG.os<>("-- Subroutine pdsyev finished with exit code ", info, '\n');
	
	delete [] work; // if only it was this easy to get rid of your work
	sca_mat_in.release();
	
	if (sca_eigvec_opt) {
		
		// convert to dbcsr matrix
		std::vector<int> rowblksizes = (m_rowblksizes_out) 
			? *m_rowblksizes_out : m_mat_in->row_blk_sizes();
			
		std::vector<int> colblksizes = (m_colblksizes_out) 
			? *m_colblksizes_out : m_mat_in->col_blk_sizes();
			
		//sca_eigvec_opt->print();
		
		matrix dbcsr_eigvec = dbcsr::scalapack_to_matrix(*sca_eigvec_opt, 
			"eigenvectors", m_world, rowblksizes, colblksizes); 
		
		m_eigvec = dbcsr_eigvec.get_smatrix();
		
		sca_eigvec_opt->release();
		
	}
		
	return;
}

#else 

void hermitian_eigen_solver::compute(int scalapack_blksize) {
	
	LOG.os<>("Running SelfAdjointEigenSolver from EIGEN.\n");
	
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver;
	
	LOG.os<>("-- Converting dbcsr matrix to eigen matrix.\n");
	
	auto eigenmat_in = dbcsr::matrix_to_eigen<double>(*m_mat_in);
	
	//std::cout << "THIS " << std::endl;
	//for (size_t i = 0; i != eigenmat_in.size(); ++i) {
	//	std::cout << eigenmat_in.data()[i] << " ";
	//}
	
	LOG.os<>("-- Starting solver.\n");
	
	solver.compute(eigenmat_in);
	
	if (solver.info() != Eigen::Success) throw std::runtime_error("Eigen hermitian eigensolver failed.");
	
	LOG.os<>("-- Solver suceeded.\n");
	auto eigval = solver.eigenvalues();
	
	m_eigval.resize(eigval.size());
	for (int i = 0; i != eigval.size(); ++i) {
		m_eigval[i] = eigval(i);
	}
	
	if (m_jobz == 'V') {
		
		LOG.os<>("-- Fetching eigenvectors.\n");
	
		auto eigvec = solver.eigenvectors();
	
		std::vector<int> rowblksizes = (m_rowblksizes_out) 
			? *m_rowblksizes_out : m_mat_in->row_blk_sizes();
			
		std::vector<int> colblksizes = (m_colblksizes_out) 
			? *m_colblksizes_out : m_mat_in->col_blk_sizes();
			
		dbcsr::world w = m_mat_in->get_world();
			
		auto dbcsr_eigvec = dbcsr::eigen_to_matrix<double>(eigvec, w,
			"eigenvectors", rowblksizes, colblksizes, dbcsr_type_no_symmetry);
			
		m_eigvec = dbcsr_eigvec.get_smatrix();
		
	}
	
	return;
	
}

#endif // SCALAPACK

smatrix hermitian_eigen_solver::inverse() {
	
	matrix eigvec_copy = matrix::copy<double>(*m_eigvec);
	
	//dbcsr::print(eigvec_copy);
	
	std::vector<double> eigval_copy = m_eigval;
	
	std::for_each(eigval_copy.begin(),eigval_copy.end(),[](double& d) { d = 1.0/d; });
	
	eigvec_copy.scale(eigval_copy, "right");
	
	//dbcsr::print(eigvec_copy);
	
	matrix inv = matrix::create_template(*m_mat_in).name(m_mat_in->name() + "^-1").type(dbcsr_type_symmetric);
	
	dbcsr::multiply('N', 'T', eigvec_copy, *m_eigvec, inv).perform(); 
	
	//eigvec_copy.release();
	
	return inv.get_smatrix();
	
}

smatrix hermitian_eigen_solver::inverse_sqrt() {
	
	matrix eigvec_copy = matrix::copy<double>(*m_eigvec);
	
	//dbcsr::print(eigvec_copy);
	
	std::vector<double> eigval_copy = m_eigval;
	
	std::for_each(eigval_copy.begin(),eigval_copy.end(),[](double& d) { d = 1.0/sqrt(d); });
	
	eigvec_copy.scale(eigval_copy, "right");
	
	//dbcsr::print(eigvec_copy);
	
	matrix inv = matrix::create_template(*m_mat_in).name(m_mat_in->name() + "^-1").type(dbcsr_type_symmetric);
	
	dbcsr::multiply('N', 'T', eigvec_copy, *m_eigvec, inv).perform(); 
	
	//eigvec_copy.release();
	
	return inv.get_smatrix();
	
}
	
} //end namepace
	
	
