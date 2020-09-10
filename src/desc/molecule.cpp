#include "desc/molecule.h"
#include "utils/mpi_log.h"
#include <utility>
#include <stdexcept>
#include <iostream>

#define printvec(LOG, n, v) \
	for (auto x : v) { \
		LOG.os<n>(x, " "); \
	} LOG.os<n>('\n');

namespace desc {
	
// Taken from PSI4
static const std::vector<int> reference_S = {  0,
											   1,                                                                                           0,
											   1, 0,                                                                         1, 2, 3, 2, 1, 0,
											   1, 0,                                                                         1, 2, 3, 2, 1, 0,
											   1, 0,                                           1, 2, 3, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0,
											   1, 0,                                           1, 2, 5, 6, 5, 4, 3, 0, 1, 0, 1, 2, 3, 2, 1, 0,
											   1, 0, 1, 0, 3, 4, 5, 6, 7, 8, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0 };

static const std::vector<int> conf_orb = {0, 2, 10, 18, 36, 54, 86, 118};

molecule::molecule(molecule::create& p) : 
	m_name(*p.c_name),
	m_mult(*p.c_mult), 
	m_charge(*p.c_charge),
	m_mo_split((p.c_mo_split) ? *p.c_mo_split : MOLECULE_MO_SPLIT),
	m_ao_split_method((p.c_ao_split_method) ? *p.c_ao_split_method : MOLECULE_AO_SPLIT_METHOD)
	{
	
	//atoms
	m_atoms = *p.c_atoms;
	
	// first: form clustered basis sets
	cluster_basis cbas(*p.c_basis,m_ao_split_method);
	
	m_cluster_basis = cbas;
	
	if (p.c_dfbasis) {
		//std::cout << "There is a df basis: " << p.c_dfbasis->size() << std::endl;
		cluster_basis cdfbas(*p.c_dfbasis,m_ao_split_method);
		
		//for (auto i : cdfbas.cluster_sizes()) {
		//	std::cout << i << std::endl;
		//}
		
		m_cluster_dfbasis = optional<cluster_basis,val>(cdfbas);
	}
	
	// secondly: occ/virt info
	
	m_frac = false;
	if (p.c_fractional) m_frac = *p.c_fractional;
	
	//if (m_frac) std::cout << "FRACTIONAL" << std::endl;
	
	if (m_frac) {
		
		if (m_atoms.size() != 1) 
			throw std::runtime_error("Fractional Occupation only for single atoms for now.");
		
		// occupied/virtual info
		int Z = m_atoms[0].atomic_number;
		
		int nbas = m_cluster_basis.nbf();
		
		// total number of electrons
		m_nele = Z - m_charge;
		
		m_mult = reference_S[Z]; // mult is overwritten
		
		if ((p.c_spin_average) ? *p.c_spin_average : true) {
			
			m_nele_alpha = m_nele_beta = 0.5 * m_nele;
			
		} else {
			
			m_nele_alpha = 0.5 * (m_nele + m_mult);
			m_nele_beta = 0.5 * (m_nele - m_mult);
		
		}
		
		auto nlimit = std::lower_bound(conf_orb.begin(), conf_orb.end(), Z);
		
		if (Z < *nlimit) --nlimit;
		
		int ncore, nact;
		if ((*nlimit) == Z) {
		 // Special case: we can hit the boundary at the end of the array
			ncore = 0;
			nact = (*nlimit) / 2;
		} else {
			ncore = (*nlimit) / 2;
			nact = (*(++nlimit)) / 2 - ncore;
		}
	
		m_nocc_alpha = m_nocc_beta = nact + ncore;
		
		double nfraca = sqrt(((double)m_nele_alpha - (double)ncore)/(double)nact);
		double nfracb = sqrt(((double)m_nele_beta - (double)ncore)/(double)nact);
		
		m_nvir_alpha= m_nvir_beta = nbas - m_nocc_alpha;
		
		// form scaled occupation vector
		std::vector<double> occ_a(m_nocc_alpha, 1.0);
		std::vector<double> occ_b(m_nocc_beta, 1.0);
		
		for (size_t i = ncore; i < m_nocc_alpha; ++i) occ_a[i] = nfraca;
		for (size_t i = ncore; i < m_nocc_beta; ++i) occ_b[i] = nfracb;
		
		m_frac_occ_alpha = optional<std::vector<double>,val>(occ_a);
		m_frac_occ_beta = optional<std::vector<double>,val>(occ_b);
		
	} else {
		
		int nbas = m_cluster_basis.nbf();
	
		// total number of electrons
		m_nele = 0;
		for (int i = 0; i != m_atoms.size(); ++i) {
			m_nele += m_atoms[i].atomic_number;
		}
		
		m_nele -= m_charge;
		int nue = m_mult - 1; // number of unpaired electrons
			
		if ((m_nele - nue) % 2 != 0) 
			throw std::runtime_error("Mult not compatible with charge.");
			
		m_nele_alpha = m_nocc_alpha = (m_nele - nue) / 2 + nue;
		m_nele_beta = m_nocc_beta = (m_nele - nue) / 2;
			
		m_nvir_alpha = nbas - m_nocc_alpha;
		m_nvir_beta = nbas - m_nocc_beta;
	
	}
	
	block_sizes blks(*this, m_mo_split);
	
	m_blocks = blks;

	
}

void molecule::print_info(MPI_Comm comm, int level) {
	
	util::mpi_log LOG(comm,level);
	
	LOG.os<0>("Printing relevant info for molecule...\n");
	LOG.os<0>("Charge/multiplicity: ", m_charge, "/", m_mult, '\n');
	LOG.os<0>("Number of alpha electrons: ", m_nele_alpha, '\n');
	LOG.os<0>("Number of beta electrons: ", m_nele_beta, '\n');
	LOG.os<0>("Number of basis functions: ", m_cluster_basis.nbf(), '\n');
	LOG.os<0>("Number of occ. alpha orbs: ", m_nocc_alpha, '\n');
	LOG.os<0>("Number of occ. beta orbs: ", m_nocc_beta, '\n');
	LOG.os<0>("Number of vir. alpha orbs: ", m_nvir_alpha, '\n');
	LOG.os<0>("Number of vir. beta orbs: ", m_nvir_beta, '\n');
	
	LOG.os<1>("Block sizes: \n");
	LOG.os<1>("occ. alpha: "); printvec(LOG, 1, m_blocks.oa());
	LOG.os<1>("occ. beta: "); printvec(LOG, 1, m_blocks.ob());
	LOG.os<1>("vir. alpha: "); printvec(LOG, 1, m_blocks.va());
	LOG.os<1>("vir. beta: "); printvec(LOG, 1, m_blocks.vb());
	LOG.os<1>("basis: "); printvec(LOG, 1, m_blocks.b());
	
	if (m_cluster_dfbasis) {
		LOG.os<1>("dfbasis: "); printvec(LOG, 1, m_blocks.x());
	}
	
	if (m_frac_occ_alpha) {
		LOG.os<1>("Frac. occ. alpha: "); printvec(LOG, 1, *m_frac_occ_alpha);
	}
	
	if (m_frac_occ_beta) {
		LOG.os<1>("Frac. occ. beta: "); printvec(LOG, 1, *m_frac_occ_beta);
	}
	
	
	LOG.os<0>('\n');
	
}

void molecule::set_dfbasis(std::vector<libint2::Shell>& dfbasis) {
	
		cluster_basis cdfbas(dfbasis,m_ao_split_method);
		
		m_cluster_dfbasis = optional<cluster_basis,val>(cdfbas);
		
		m_blocks.set_x(*m_cluster_dfbasis);
	
}

}