#include "MantidKernel/WarningSuppressions.h"
//------------------------------------------------------------------------------
// This is a global 'include' file for the GSoap generated files in order to
// disable some warnings that we understand but do not want to touch as
// it is automatically generated code
//------------------------------------------------------------------------------
GCC_DIAG_OFF(cast - qual)
GCC_DIAG_OFF(conversion)
GCC_DIAG_OFF(unused - parameter)
GCC_DIAG_OFF(strict - aliasing)
GCC_DIAG_OFF(format)
GCC_DIAG_OFF(vla)
#if GCC_VERSION >= 40800 // 4.8.0
GCC_DIAG_OFF(literal - suffix)
#endif
#ifdef _WIN32
#pragma warning(disable : 4100)
#endif

#include "GSoapGenerated/ICat4C.cpp"
#include "GSoapGenerated/ICat4ICATPortBindingProxy.cpp"
