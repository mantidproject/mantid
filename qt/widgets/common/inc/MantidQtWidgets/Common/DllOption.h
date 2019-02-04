// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_COMMON_DLLOPTION_H_
#define MANTIDQT_COMMON_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_COMMON
#define EXPORT_OPT_MANTIDQT_COMMON DLLExport
#define EXTERN_MANTIDQT_COMMON
#else
#define EXPORT_OPT_MANTIDQT_COMMON DLLImport
#define EXTERN_MANTIDQT_COMMON extern
#endif /* IN_MANTIDQT_COMMON */

#endif // MANTIDQT_COMMON_DLLOPTION_H_
