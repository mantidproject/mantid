// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_DLLOPTION_H_
#define MANTIDQT_MANTIDWIDGETS_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_MANTIDWIDGETS
#define EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DLLExport
#else
#define EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DLLImport
#endif /* IN_MANTIDQT_MANTIDWIDGETS */

#endif // MANTIDQT_MANTIDWIDGETS_DLLOPTION_H_
