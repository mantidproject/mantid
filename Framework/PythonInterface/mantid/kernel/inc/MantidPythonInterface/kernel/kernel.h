// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
/*
 * Provides a function to initialize the numpy c api
 * function pointer table in the kernel module. This
 * is *only* required for the C++ unit tests on Windows.
 *
 * Normally, importing mantid.kernel into Python causes
 * the internal numpy array api to be initialized using
 * the _import_array call in the module startup code. This
 * assumes that the extension module itself is only loaded
 * by the Python dynamic loader and not the operating system
 * library loader as a dependency on another executable. In
 * our Python C++ unit tests the _kernel.pyd library is
 * linked to the unittest executable and is therefore loaded
 * by the OS. On Windows each DLL has a private symbol table
 * and importing mantid.kernel as part of the unit test
 * only intializes the numpy c api for that copy of the
 * shared library. The unittest executable also sees a secondary
 * copy from the dynamic linking. The C API pointer for this
 * copy of the library also needs initializing and this function
 * provides this capability.
 */
#ifdef _WIN32

#include "MantidPythonInterface/kernel/DllConfig.h"

PYTHON_KERNEL_DLL void kernel_dll_import_numpy_capi_for_unittest();
#endif
