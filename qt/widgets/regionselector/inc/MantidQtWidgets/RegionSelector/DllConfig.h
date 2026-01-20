// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_REGIONSELECTOR
#define MANTID_REGIONSELECTOR_DLL DLLExport
#else
#define MANTID_REGIONSELECTOR_DLL DLLImport
#endif /* IN_MANTIDQT_REGIONSELECTOR */
