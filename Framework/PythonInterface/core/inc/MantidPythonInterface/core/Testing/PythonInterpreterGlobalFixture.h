// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PYTHONINTERFACECPP_PYTHONINTERPRETERGLOBALFIXTURE_H
#define PYTHONINTERFACECPP_PYTHONINTERPRETERGLOBALFIXTURE_H

#include "MantidKernel/ConfigService.h"
#include "MantidPythonInterface/core/NDArray.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include "cxxtest/GlobalFixture.h"
#include <boost/algorithm/string/trim.hpp>

/**
 * PythonInterpreter
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * Python
 */
class PythonInterpreterGlobalFixture : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    using Mantid::Kernel::ConfigService;
    namespace py = boost::python;

    Py_Initialize();
    PyEval_InitThreads();
    Mantid::PythonInterface::importNumpy();
    // Insert the directory of the properties file as a sitedir
    // to ensure the built copy of mantid gets picked up
    const py::object siteModule{py::handle<>(PyImport_ImportModule("site"))};
    siteModule.attr("addsitedir")(ConfigService::Instance().getPropertiesDir());

    // Use Agg backend for matplotlib
    py::object mpl{py::handle<>(PyImport_ImportModule("matplotlib"))};
    mpl.attr("use")("Agg");

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

#endif // PYTHONINTERFACECPP_PYTHONINTERPRETERGLOBALFIXTURE_H
