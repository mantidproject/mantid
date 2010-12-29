#ifndef MANTID_ICAT_DLLEXPORT_H_
#define MANTID_ICAT_DLLEXPORT_H_
/**
*	Sets the dll import export correctly for members of the API DLL
**/

#ifdef IN_MANTID_ICAT
#define EXPORT_OPT_MANTID_ICAT DLLExport 
#else
#define EXPORT_OPT_MANTID_ICAT DLLImport
#endif /* IN_MANTID_ICAT */

#endif /*MANTID_ICAT_DLLEXPORT*/
