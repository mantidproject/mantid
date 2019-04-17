// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_DLLOPTION_H_
#define MANTIDQT_PLOTTING_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_PLOTTING
#define EXPORT_OPT_MANTIDQT_PLOTTING DLLExport
#define EXTERN_MANTIDQT_PLOTTING
#else
#define EXPORT_OPT_MANTIDQT_PLOTTING DLLImport
#define EXTERN_MANTIDQT_PLOTTING extern
#endif /* IN_MANTIDQT_PLOTTING */

#endif // MANTIDQT_PLOTTING_DLLOPTION_H_
