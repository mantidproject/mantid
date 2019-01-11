// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PARALLEL_DLLCONFIG_H_
#define MANTID_PARALLEL_DLLCONFIG_H_

/*
  This file contains the DLLExport/DLLImport linkage configuration for the
  Parallel library
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_PARALLEL
#define MANTID_PARALLEL_DLL DLLExport
#else
#define MANTID_PARALLEL_DLL DLLImport
#endif // IN_MANTID_PARALLEL

#endif // MANTID_PARALLEL_DLLCONFIG_H_
