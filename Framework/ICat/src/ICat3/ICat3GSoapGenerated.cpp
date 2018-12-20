#include "MantidKernel/WarningSuppressions.h"
//------------------------------------------------------------------------------
// This is a global 'include' file for the GSoap generated files in order to
// disable some warnings that we understand but do not want to touch as
// it is automatically generated code
//------------------------------------------------------------------------------
GNU_DIAG_OFF("cast-qual")
GNU_DIAG_OFF("conversion")
GNU_DIAG_OFF("unused-parameter")
GNU_DIAG_OFF("strict-aliasing")
GNU_DIAG_OFF("format")
GNU_DIAG_OFF("vla")
#if GCC_VERSION >= 40800 // 4.8.0
GNU_DIAG_OFF("literal-suffix")
#endif
#ifdef _WIN32
#pragma warning(disable : 4100)
#endif

#include "GSoapGenerated/ICat3C.cpp"
#include "GSoapGenerated/ICat3ICATPortBindingProxy.cpp"
