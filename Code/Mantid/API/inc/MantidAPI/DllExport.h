#ifndef MANTID_API_DLLEXPORT_H_
#define MANTID_API_DLLEXPORT_H_
/**
*	Sets the dll import export correctly for members of the API DLL
**/
#include <MantidKernel/System.h>

#ifdef IN_MANTID_API
#define EXPORT_OPT_MANTID_API DLLExport 
#else
#define EXPORT_OPT_MANTID_API DLLImport
#endif /* IN_MANTID_API */

#endif /*MANTID_API_DLLEXPORT*/
