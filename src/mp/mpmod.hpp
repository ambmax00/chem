#ifndef MPMOD_H
#define MPMOD_H

#include "megalochem.hpp"
#include "mp/mp_wfn.hpp"
#include "desc/options.hpp"
#include "utils/mpi_time.hpp"
#include <dbcsr_common.hpp>

namespace megalochem {

namespace mp {

class mpmod {
private:

	hf::shared_hf_wfn m_hfwfn;
	desc::options m_opt;
	world m_world;
	
	util::mpi_time TIME;
	util::mpi_log LOG;
	
	smp_wfn m_mpwfn;
	
public:

	mpmod(world w, hf::shared_hf_wfn wfn_in, desc::options& opt_in);
	~mpmod() {}
	
	void compute();
	
	smp_wfn wfn() {
		return m_mpwfn;
	}
	
};

} // namespace mp

} // namespace megalochem

#endif
