#include "desc/io.h"
#include <Eigen/Core>

namespace desc {

bool fexists(const std::string& filename) {
		std::ifstream ifile(filename.c_str());
		return (bool)ifile;
}

void write_matrix(dbcsr::smatrix<double>& m_in, std::string molname) {
		
		std::ofstream file;
				
		std::string file_name = molname + "_" + m_in->name() + ".dat";
		
		if (fexists(file_name) && m_in->get_world().rank() == 0) std::remove(file_name.c_str());
		
		auto eigen_mat = dbcsr::matrix_to_eigen(*m_in);
		
		if (m_in->get_world().rank() == 0) write_binary_mat(file_name.c_str(), eigen_mat);
		
}

dbcsr::smatrix<double> read_matrix(std::string molname, std::string matname, 
	dbcsr::world wrld, vec<int> rowblksizes, vec<int> colblksizes, char type) {
	
	std::ifstream file;
	
	std::string file_name = molname + "_" + matname + ".dat";
	
	if (!fexists(file_name)) throw std::runtime_error("File " + file_name + " does not exist.");
	
	Eigen::MatrixXd eigen_mat;
	
	read_binary_mat(file_name.c_str(), eigen_mat);
	
	//std::cout << tensorname << std::endl;
	//std::cout << eigen_mat << std::endl;
	
	//dbcsr::pgrid<2> grid2(comm);
	return (dbcsr::eigen_to_matrix(eigen_mat, wrld, matname, rowblksizes, colblksizes, type)).get_smatrix();
	
}

void write_vector(svector<double>& v_in, std::string molname, std::string vecname, MPI_Comm comm) {
	
		std::ofstream file;
		
		int myrank = -1;
		
		MPI_Comm_rank(comm, &myrank);
				
		std::string file_name = molname + "_" + vecname + ".dat";
		
		if (fexists(file_name) && myrank == 0) std::remove(file_name.c_str());
		
		Eigen::VectorXd eigen_vec = Eigen::Map<Eigen::VectorXd, Eigen::Unaligned>(v_in->data(), v_in->size());
		
		if (myrank == 0) write_binary_mat(file_name.c_str(), eigen_vec);
		
}	

void read_vector(svector<double>& v_in, std::string molname, std::string vecname) {
	
	std::ifstream file;
	
	std::string file_name = molname + "_" + vecname + ".dat";
	
	if (!fexists(file_name)) throw std::runtime_error("File " + file_name + " does not exist.");
	
	Eigen::VectorXd eigen_vec;
	
	read_binary_mat(file_name.c_str(), eigen_vec);
	
	v_in = std::make_shared<std::vector<double>>(
		std::vector<double>(eigen_vec.data(), eigen_vec.data() + eigen_vec.size()));
	
	
}
	

} // end namesapce 
