// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PYTHONINTERFACE_CPP_GLOBALTESTINITIALIZATION_H
#define PYTHONINTERFACE_CPP_GLOBALTESTINITIALIZATION_H

#include "MantidPythonInterface/core/Testing/PythonInterpreterGlobalFixture.h"
#ifdef _WIN32
#include "MantidPythonInterface/kernel/kernel.h"
#endif

/**
 * Import Numpy C API on Windows. This is a noop on other platforms.
 * See MantidPythonInterface/kernel/kernel.h for the full details
 * of why this is required.
 */
class ImportNumpyCAPI : CxxTest::GlobalFixture {

  bool setUpWorld() override {
#ifdef _WIN32
    kernel_dll_import_numpy_capi_for_unittest();
#endif
    return true;
  }
};

//------------------------------------------------------------------------------
// We rely on cxxtest only including this file once so that the following
// statements do not cause multiple-definition errors.
//------------------------------------------------------------------------------
static PythonInterpreterGlobalFixture PYTHON_INTERPRETER;
static ImportNumpyCAPI IMPORT_NUMPY_C_API_KERNEL;

#endif // PYTHONINTERFACE_CPP_GLOBALTESTINITIALIZATION_H
