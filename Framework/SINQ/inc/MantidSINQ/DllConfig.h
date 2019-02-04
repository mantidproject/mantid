// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_DLLCONFIG_H_
#define MANTID_SINQ_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    SINQ library
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_SINQ
#define MANTID_SINQ_DLL DLLExport
#define EXTERN_MANTID_SINQ
#else
#define MANTID_SINQ_DLL DLLImport
#define EXTERN_MANTID_SINQ EXTERN_IMPORT
#endif /* IN_MANTID_SINQ */

#endif // MANTID_SINQ_DLLCONFIG_H_
