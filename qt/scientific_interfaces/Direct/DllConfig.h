// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    MantidQt CustomInterfaces library
*/
#ifdef IN_MANTIDQT_DIRECT
#define MANTIDQT_DIRECT_DLL DLLExport
#define EXTERN_MANTIDQT_DIRECT
#else
#define MANTIDQT_DIRECT_DLL DLLImport
#define EXTERN_MANTIDQT_DIRECT EXTERN_IMPORT
#endif
