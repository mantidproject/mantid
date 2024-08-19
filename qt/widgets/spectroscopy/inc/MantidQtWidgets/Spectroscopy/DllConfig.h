// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_SPECTROSCOPY
#define MANTID_SPECTROSCOPY_DLL DLLExport
#else
#define MANTID_SPECTROSCOPY_DLL DLLImport
#endif /* IN_MANTIDQT_SPECTROSCOPY */
