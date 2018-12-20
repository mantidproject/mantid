//------------------------------------------------------------------------------
// This is a global 'include' file for the #include "ANN cpp files in order to
// disable some warnings that we understand but do not want to touch as
// it is not our code
//------------------------------------------------------------------------------
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include "ANN/ANN.cpp"
#include "ANN/bd_fix_rad_search.cpp"
#include "ANN/bd_pr_search.cpp"
#include "ANN/bd_search.cpp"
#include "ANN/bd_tree.cpp"
#include "ANN/bd_tree.h"
#include "ANN/brute.cpp"
#include "ANN/kd_dump.cpp"
#include "ANN/kd_fix_rad_search.cpp"
#include "ANN/kd_pr_search.cpp"
#include "ANN/kd_search.cpp"
#include "ANN/kd_split.cpp"
#include "ANN/kd_tree.cpp"
#include "ANN/kd_util.cpp"
#include "ANN/perf.cpp"
