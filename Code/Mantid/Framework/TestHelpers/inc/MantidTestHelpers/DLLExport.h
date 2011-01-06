#ifndef MANTID_TESTHELPERS_DLLEXPORT_H_
#define MANTID_TESTHELPERS_DLLEXPORT_H_
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/System.h"

#ifdef IN_MANTID_TESTHELPERS
#define DLL_TESTHELPERS DLLExport
#else
#define DLL_TESTHELPERS DLLImport
#endif

#endif //MANTID_TESTHELPERS_DLLEXPORT_H_
