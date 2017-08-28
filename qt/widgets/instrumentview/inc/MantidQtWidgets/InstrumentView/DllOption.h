#ifndef MANTIDQT_INSTRUMENTVIEW_DLLOPTION_H_
#define MANTIDQT_INSTRUMENTVIEW_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_INSTRUMENTVIEW
#define EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW DLLExport
#define EXTERN_MANTIDQT_INSTRUMENTVIEW
#else
#define EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW DLLImport
#define EXTERN_MANTIDQT_INSTRUMENTVIEW extern
#endif /* IN_MANTIDQT_INSTRUMENTVIEW */

#endif // MANTIDQT_INSTRUMENTVIEW_DLLOPTION_H_
