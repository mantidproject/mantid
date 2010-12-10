#ifndef MANTID_GEOMETRY_DLLEXPORT_H_
#define MANTID_GEOMETRY_DLLEXPORT_H_
/**
*	Sets the dll import export correctly for members of the KERNEL DLL
**/
#include <MantidKernel/System.h>

#ifdef IN_MANTID_GEOMETRY
#define EXPORT_OPT_MANTID_GEOMETRY DLLExport 
#else
#define EXPORT_OPT_MANTID_GEOMETRY DLLImport
#endif /* IN_MANTID_GEOMETRY*/

#endif /*MANTID_GEOMETRY_DLLEXPORT*/
