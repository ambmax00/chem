#ifndef MP_DEFAULTS_H
#define MP_DEFAULTS_H

#include <string>

namespace mp {
	
inline const int MP_PRINT_LEVEL = 0;
inline const int MP_NLAP = 5;
inline const int MP_NBATCHES_X = 4;
inline const int MP_NBATCHES_B = 4;

inline const double MP_C_OS = 1.3;
inline const double MP_CHOLPREC = 1e-12;

inline const std::string MP_ERIS = "core";
inline const std::string MP_INTERMEDS = "core";
inline const std::string MP_BUILD_Z = "LLMPFULL";

inline const bool MP_FORCE_SPARSITY = true;

}

#endif