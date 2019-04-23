// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ICONS_DLLOPTION_H_
#define MANTIDQT_ICONS_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_ICONS
#define EXPORT_OPT_MANTIDQT_ICONS DLLExport
#define EXTERN_MANTIDQT_ICONS
#else
#define EXPORT_OPT_MANTIDQT_ICONS DLLImport
#define EXTERN_MANTIDQT_ICONS extern
#endif /* IN_MANTIDQT_ICONS */

#endif // MANTIDQT_ICONS_DLLOPTION_H_
