#ifndef MANTIDQT_API_DLLOPTION_H_
#define MANTIDQT_API_DLLOPTION_H_

#ifndef DLLExport
  #ifdef _WIN32
    #define DLLExport __declspec( dllexport )
    #define DLLImport __declspec( dllimport )
  #else
    #define DLLExport
    #define DLLImport
  #endif
#endif

#ifdef IN_MANTIDQT_API
#define EXPORT_OPT_MANTIDQT_API DLLExport 
#else
#define EXPORT_OPT_MANTIDQT_API DLLImport
#endif /* IN_MANTIDQT_API */

#endif //MANTIDQT_API_DLLOPTION_H_
