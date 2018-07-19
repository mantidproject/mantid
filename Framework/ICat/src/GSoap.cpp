#include "MantidKernel/WarningSuppressions.h"
//------------------------------------------------------------------------------
// This is a global 'include' file for the GSoap generated files in order to
// disable some warnings that we understand but do not want to touch as
// it is automatically generated code
//------------------------------------------------------------------------------
// clang-format off
DIAG_OFF(cast-qual)
DIAG_OFF(conversion)
DIAG_OFF(unused-parameter)
DIAG_OFF(strict-aliasing)
DIAG_OFF(format)
DIAG_OFF(vla)
// clang-format on
#if GCC_VERSION >= 40800 // 4.8.0
// clang-format off
DIAG_OFF(literal-suffix)
// clang-format on
#endif
#ifdef _WIN32
#pragma warning(disable : 4100)
#endif

#include "GSoap/soapserializersC.cpp"
#include "GSoap/stdsoap2.cpp"
