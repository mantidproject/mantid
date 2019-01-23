// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_LEGACYQWT_DLLOPTION_H_
#define MANTIDQT_LEGACYQWT_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_LEGACYQWT
#define EXPORT_OPT_MANTIDQT_LEGACYQWT DLLExport
#define EXTERN_MANTIDQT_LEGACYQWT
#else
#define EXPORT_OPT_MANTIDQT_LEGACYQWT DLLImport
#define EXTERN_MANTIDQT_LEGACYQWT extern
#endif /* IN_MANTIDQT_LEGACYQWT */

#endif // MANTIDQT_LEGACYQWT_DLLOPTION_H_
