// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTID_SPECTRUM_VIEWER_DLLOPTION_H_
#define MANTIDQT_MANTID_SPECTRUM_VIEWER_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_SPECTRUMVIEWER
#define EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER DLLExport
#else
#define EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER DLLImport
#endif

#endif // MANTIDQT_MANTID_SPECTRUM_VIEWER_DLLOPTION_H_
