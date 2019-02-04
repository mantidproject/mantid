// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ISISREFLECTOMETRY_DLLCONFIG_H_
#define MANTIDQT_ISISREFLECTOMETRY_DLLCONFIG_H_

#include "MantidKernel/System.h"

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    MantidQt CustomInterfaces library
*/
#ifdef IN_MANTIDQT_ISISREFLECTOMETRY
#define MANTIDQT_ISISREFLECTOMETRY_DLL DLLExport
#define EXTERN_MANTIDQT_ISISREFLECTOMETRY
#else
#define MANTIDQT_ISISREFLECTOMETRY_DLL DLLImport
#define EXTERN_MANTIDQT_ISISREFLECTOMETRY EXTERN_IMPORT
#endif

#endif // MANTIDQT_ISISREFLECTOMETRY_DLLCONFIG_H_
