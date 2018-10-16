// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_DLLCONFIG_H_
#define MANTID_INDEXING_DLLCONFIG_H_

/*
  This file contains the DLLExport/DLLImport linkage configuration for the
  Indexing library
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_INDEXING
#define MANTID_INDEXING_DLL DLLExport
#else
#define MANTID_INDEXING_DLL DLLImport
#endif // IN_MANTID_INDEXING

#endif // MANTID_INDEXING_DLLCONFIG_H_
