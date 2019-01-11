// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_DLLCONFIG_H_
#define MANTIDQTCUSTOMINTERFACES_DLLCONFIG_H_

#include "MantidKernel/System.h"

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    MantidQt CustomInterfaces library
*/
#ifdef IN_MANTIDQT_MUONINTERFACE
#define MANTIDQT_MUONINTERFACE_DLL DLLExport
#define EXTERN_MANTIDQT_MUONINTERFACE
#else
#define MANTIDQT_MUONINTERFACE_DLL DLLImport
#define EXTERN_MANTIDQT_MUONINTERFACE EXTERN_IMPORT
#endif

#endif // MANTIDQTCUSTOMINTERFACES_DLLCONFIG_H_
