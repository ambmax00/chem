#include <vector>
#include <stdexcept>
#include "ints/aofactory.h"
#include "ints/integrals.h"
#include "ints/screening.h"
#include "utils/pool.h"
#include "utils/params.hpp"
#include <libint2.hpp>

#include <iostream>
#include <limits>

namespace ints {

template <int N>
dbcsr::tensor<N,double> aofactory::compute(aofac_params&& p) {
		
		std::string Op = *p.op;
		std::string bis = *p.bas;
		
		libint2::initialize();
		
		libint2::Operator libOp = libint2::Operator::invalid;
		libint2::BraKet libBraKet = libint2::BraKet::invalid;
		
		if (Op == "coulomb") libOp = libint2::Operator::coulomb;
		if (Op == "overlap") libOp = libint2::Operator::overlap;
		if (Op == "kinetic") libOp = libint2::Operator::kinetic;
		if (Op == "nuclear") libOp = libint2::Operator::nuclear;
		if (Op == "erfc_coulomb") libOp = libint2::Operator::erfc_coulomb;
		
		if (libOp == libint2::Operator::invalid) 
			throw std::runtime_error("Invalid operator: "+Op);
		
		vec<desc::cluster_basis> basvec;
		
		desc::cluster_basis c_bas = m_mol.c_basis();
		optional<desc::cluster_basis,val> x_bas = m_mol.c_dfbasis();
		
		if (bis == "bb") { 
			basvec = {c_bas, c_bas};
		} else if (bis == "xx") {
			basvec = {*x_bas, *x_bas};
			libBraKet = libint2::BraKet::xs_xs;
		} else if (bis == "xbb") {
			basvec = {*x_bas, c_bas, c_bas};
			libBraKet = libint2::BraKet::xs_xx;
		} else if (bis == "bbbb") {
			std::cout << "4" << std::endl;
			basvec = {c_bas, c_bas, c_bas, c_bas};
			libBraKet = libint2::BraKet::xx_xx;
		} else {
			throw std::runtime_error("Unsupported basis set specifications: "+bis);
		}
		
		size_t max_nprim = 0;
		int max_l = 0;
		
		for (int i = 0; i != basvec.size(); ++i) {
			max_nprim = std::max(basvec[i].max_nprim(), max_nprim);
			max_l = std::max(basvec[i].max_l(), max_l);
		}
		
		//std::cout << "MAX " << max_nprim << " " << max_l << std::endl;
		
		libint2::Engine eng(libOp, max_nprim, max_l, 0, std::numeric_limits<double>::epsilon());
			
		if (libBraKet != libint2::BraKet::invalid) {
			eng.set(libBraKet);
		}
		
		// OPTIONS
		if (Op == "nuclear") {
			eng.set_params(make_point_charges(m_mol.atoms())); 
		} else if (Op == "erfc_coulomb") {
			double omega = 0.1; //*ctx.get<double>("INT/omega");
			eng.set_params(omega);
		}
			
		util::ShrPool<libint2::Engine> eng_pool = util::make_pool<libint2::Engine>(eng);
		
		std::cout << "Op: " << Op << " bis: " << bis << std::endl;
		
		
		std::cout << "COMPUTE SCREENING: " << std::endl;
		
		libint2::Engine screen(libint2::Operator::coulomb, max_nprim, max_l, 0, std::numeric_limits<double>::epsilon());
		screen.set(libint2::BraKet::xx_xx);
		util::ShrPool<libint2::Engine> eng2_pool = util::make_pool<libint2::Engine>(screen);
		
		Zmat Z(m_comm, m_mol, eng2_pool, "schwarz");
		
		Z.compute();
		
		exit(0);
		
		dbcsr::tensor<N,double> out = integrals<N>(m_comm, eng_pool, basvec, *p.name, *p.map1, *p.map2);
		
		libint2::finalize();
		
		return out;
		
}

//forward declarations
template dbcsr::tensor<2,double> aofactory::compute(aofac_params&& p);
//template dbcsr::tensor<3,double> aofactory::compute(aofac_params&& p);
template dbcsr::tensor<4,double> aofactory::compute(aofac_params&& p);


} // end namespace ints
