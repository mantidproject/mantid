#ifndef MANTIDQT_MANTIDWIDGETS_DLLOPTION_H_
#define MANTIDQT_MANTIDWIDGETS_DLLOPTION_H_

#ifndef DLLExport
  #ifdef _WIN32
    #define DLLExport __declspec( dllexport )
    #define DLLImport __declspec( dllimport )
  #else
    #define DLLExport
    #define DLLImport
  #endif
#endif

#ifdef IN_MANTIDQT_MANTIDWIDGETS
#define EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DLLExport 
#else
#define EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DLLImport
#endif /* IN_MANTIDQT_API */

#endif //MANTIDQT_API_DLLOPTION_H_