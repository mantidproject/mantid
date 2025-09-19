// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/PythonObjectTypeValidator.h"
#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::IValidator;
using Mantid::PythonInterface::PythonObjectTypeValidator;
using namespace boost::python;

namespace {
PythonObjectTypeValidator *createPythonObjectTypeValidator(object pythonClass) {
  return new PythonObjectTypeValidator(pythonClass);
}
} // namespace

void export_PythonObjectTypeValidator() {
  class_<PythonObjectTypeValidator, bases<IValidator>, boost::noncopyable>("PythonObjectTypeValidator")
      .def("__init__",
           make_constructor(&createPythonObjectTypeValidator, default_call_policies(), (arg("PythonClass"))),
           "Constructs a validator verifying that objects passed to this property are of the given class")
      .def("isValid", &PythonObjectTypeValidator::isValid<boost::python::object>);
}
