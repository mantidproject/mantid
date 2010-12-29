#ifndef MANTID_KERNEL_DLLEXPORT_H_
#define MANTID_KERNEL_DLLEXPORT_H_
/**
*	Sets the dll import export correctly for members of the KERNEL DLL
**/
#include <MantidKernel/System.h>

#ifdef IN_MANTID_KERNEL
#define EXPORT_OPT_MANTID_KERNEL DLLExport 
#else
#define EXPORT_OPT_MANTID_KERNEL DLLImport
#endif /* IN_MANTID_KERNEL*/

#endif /*MANTID_KERNEL_DLLEXPORT*/
