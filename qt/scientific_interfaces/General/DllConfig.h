// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTINTERFACESGENERAL_DLLCONFIG_H_
#define MANTIDQTINTERFACESGENERAL_DLLCONFIG_H_

#include "MantidKernel/System.h"

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    MantidQt CustomInterfaces library
*/
#ifdef IN_MANTIDQT_INTERFACESGENERAL
#define MANTIDQT_INTERFACESGENERAL_DLL DLLExport
#define EXTERN_MANTIDQT_INTERFACESGENERAL
#else
#define MANTIDQT_INTERFACESGENERAL_DLL DLLImport
#define EXTERN_MANTIDQT_INTERFACESGENERAL EXTERN_IMPORT
#endif

#endif // MANTIDQTINTERFACESGENERAL_DLLCONFIG_H_
