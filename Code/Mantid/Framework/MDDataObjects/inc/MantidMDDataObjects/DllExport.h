#ifndef MANTID_MDDATAOBJECTS_DLLEXPORT_H_
#define MANTID_MDDATAOBJECTS_DLLEXPORT_H_
/**
*	Sets the dll import export correctly for members of the KERNEL DLL
**/
#include <MantidKernel/System.h>

#ifdef IN_MANTID_MDDATAOBJECTS
#define EXPORT_OPT_MANTID_MDDATAOBJECTS DLLExport 
#else
#define EXPORT_OPT_MANTID_MDDATAOBJECTS DLLImport
#endif /* IN_MANTID_MDDATAOBJECTS*/

#endif /*MANTID_MDDATAOBJECTS_DLLEXPORT_H_*/
