#ifndef FOCK_DEFAULTS_H
#define FOCK_DEFAULTS_H

#include <string>

namespace fock {
	
	inline const int FOCK_PRINT_LEVEL = 0;
	
	inline const std::string FOCK_BUILD_J = "exact";
	inline const std::string FOCK_BUILD_K = "exact";
	inline const std::string FOCK_METRIC = "coulomb";
	inline const std::string FOCK_ERIS = "core";
	
} // end namespace

#endif
