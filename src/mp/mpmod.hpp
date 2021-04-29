#ifndef MPMOD_H
#define MPMOD_H

#include "mp/mp_wfn.hpp"
#include "desc/options.hpp"
#include "utils/mpi_time.hpp"
#include <dbcsr_common.hpp>

namespace mp {

class mpmod {
private:

	hf::shared_hf_wfn m_hfwfn;
	desc::options m_opt;
	dbcsr::cart m_cart;
	
	util::mpi_time TIME;
	util::mpi_log LOG;
	
	smp_wfn m_mpwfn;
	
public:

	mpmod(dbcsr::cart w, hf::shared_hf_wfn wfn_in, desc::options& opt_in);
	~mpmod() {}
	
	void compute();
	
	smp_wfn wfn() {
		return m_mpwfn;
	}
	
};

}

#endif
