// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DLLCONFIG_H_
#define MANTID_KERNEL_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
   Kernel library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_KERNEL
#define MANTID_KERNEL_DLL DLLExport
#define EXTERN_MANTID_KERNEL
#else
#define MANTID_KERNEL_DLL DLLImport
#define EXTERN_MANTID_KERNEL EXTERN_IMPORT
#endif /* IN_MANTID_KERNEL*/

#endif // MANTID_KERNEL_DLLCONFIG_H_
