// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    MantidQt CustomInterfaces library
*/
#ifdef IN_MANTIDQT_ENGGDIFFRACTION
#define MANTIDQT_ENGGDIFFRACTION_DLL DLLExport
#define EXTERN_MANTIDQT_ENGGDIFFRACTION
#else
#define MANTIDQT_ENGGDIFFRACTION_DLL DLLImport
#define EXTERN_MANTIDQT_ENGGDIFFRACTION EXTERN_IMPORT
#endif
