// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_FITTING
#define EXPORT_OPT_MANTIDQT_FITTING DLLExport
#define EXTERN_MANTIDQT_FITTING
#else
#define EXPORT_OPT_MANTIDQT_FITTING DLLImport
#define EXTERN_MANTIDQT_FITTING extern
#endif /* IN_MANTIDQT_FITTING */
