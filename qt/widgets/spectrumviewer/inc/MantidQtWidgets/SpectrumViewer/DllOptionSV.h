// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_SPECTRUMVIEWER
#define EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER DLLExport
#else
#define EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER DLLImport
#endif
