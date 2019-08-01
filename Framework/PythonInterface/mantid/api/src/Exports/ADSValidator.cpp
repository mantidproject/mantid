// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ADSValidator.h"
#include "MantidKernel/TypedValidator.h"
#include "MantidPythonInterface/core/TypedValidatorExporter.h"
#include <boost/python/class.hpp>

using Mantid::API::ADSValidator;
using Mantid::Kernel::TypedValidator;
using Mantid::PythonInterface::TypedValidatorExporter;
using namespace boost::python;

/// This is the base TypedValidator for most of the WorkspaceValidators
void export_ADSValidator() {
  TypedValidatorExporter<std::string>::define("StringTypedValidator");

  class_<ADSValidator, bases<TypedValidator<std::string>>, boost::noncopyable>(
      "ADSValidator", init<>("Default constructor"))
      .def(init<const bool, const bool>(
          "Constructor setting allowMultiple and isOptional.",
          args("allowMultipleSelection", "isOptional")))

      .def("isMultipleSelectionAllowed",
           &ADSValidator::isMultipleSelectionAllowed, (arg("self")),
           "Are multiple workspaces allowed")
      .def("setMultiSelectionAllowed",
           &ADSValidator::setMultipleSelectionAllowed,
           (arg("self"), arg("isMultiSelectionAllowed")),
           "Set if multiple workspaces are allowed")
      .def("isOptional", &ADSValidator::isOptional, (arg("self")),
           "Is this optional")
      .def("setOptional", &ADSValidator::setOptional,
           (arg("self"), arg("setOptional")), "Set if this is optional");
}
