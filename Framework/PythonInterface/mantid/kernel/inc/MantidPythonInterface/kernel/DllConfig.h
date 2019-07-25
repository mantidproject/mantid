// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_KERNEL_DLLCONFIG_H_
#define MANTID_PYTHONINTERFACE_KERNEL_DLLCONFIG_H_

/*
    This file contains the DLLExport/DLLImport linkage configuration for the
    Python _kernel library

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/System.h"

#ifdef PythonKernelModule_EXPORTS
#define PYTHON_KERNEL_DLL DLLExport
#else
#define PYTHON_KERNEL_DLL DLLImport
#endif

#endif // MANTID_PYTHONINTERFACE_KERNEL_DLLCONFIG_H_
