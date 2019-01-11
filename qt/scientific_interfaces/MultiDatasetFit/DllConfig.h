// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMULTIDATASETFIT_DLLCONFIG_H_
#define MANTIDQTMULTIDATASETFIT_DLLCONFIG_H_

#include "MantidKernel/System.h"

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    MantidQt CustomInterfaces library
*/
#ifdef IN_MANTIDQT_MULTIDATASETFIT
#define MANTIDQT_MULTIDATASETFIT_DLL DLLExport
#define EXTERN_MANTIDQT_MULTIDATASETFIT
#else
#define MANTIDQT_MULTIDATASETFIT_DLL DLLImport
#define EXTERN_MANTIDQT_MULTIDATASETFIT EXTERN_IMPORT
#endif

#endif // MANTIDQTMULTIDATASETFIT_DLLCONFIG_H_
