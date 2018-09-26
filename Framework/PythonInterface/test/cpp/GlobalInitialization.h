#ifndef PYTHONINTERFACECPP_GLOBALINITIALIZATION_H
#define PYTHONINTERFACECPP_GLOBALINITIALIZATION_H

#include "MantidKernel/ConfigService.h"
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#ifdef _WIN32
#include "MantidPythonInterface/kernel/kernel.h"
#endif
#include "cxxtest/GlobalFixture.h"
#include <boost/algorithm/string/trim.hpp>
/**
 * PythonInterpreter
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * Python
 */
class PythonInterpreter : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    using Mantid::Kernel::ConfigService;
    using Mantid::PythonInterface::importNumpy;
    using boost::algorithm::trim_right_copy_if;

    Py_Initialize();
    importNumpy();
    // add location of mantid module to sys.path
    std::string propDir =
        trim_right_copy_if(ConfigService::Instance().getPropertiesDir(),
                           [](char c) { return (c == '/' || c == '\\'); });
    const std::string appendMantidToPath = "import sys\n"
                                           "sys.path.insert(0, '" +
                                           propDir + "')";
    PyRun_SimpleString(appendMantidToPath.c_str());
#ifdef _WIN32
    // See kernel.h for the explanation
    kernel_dll_import_numpy_capi_for_unittest();
#endif

    return Py_IsInitialized();
  }

  bool tearDown() override {
    // Some test methods may leave the Python error handler with an error
    // set that confuse other tests when the executable is run as a whole
    // Clear the errors after each suite method is run
    PyErr_Clear();
    return CxxTest::GlobalFixture::tearDown();
  }

  bool tearDownWorld() override {
    Py_Finalize();
    return true;
  }
};

//------------------------------------------------------------------------------
// We rely on cxxtest only including this file once so that the following
// statements do not cause multiple-definition errors.
//------------------------------------------------------------------------------
static PythonInterpreter PYTHON_INTERPRETER;

#endif // PYTHONINTERFACECPP_GLOBALINITIALIZATION_H
