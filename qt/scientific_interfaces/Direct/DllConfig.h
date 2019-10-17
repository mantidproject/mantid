// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_DIRECT_DLLCONFIG_H_
#define MANTIDQT_DIRECT_DLLCONFIG_H_

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

#endif // MANTIDQT_DIRECT_DLLCONFIG_H_