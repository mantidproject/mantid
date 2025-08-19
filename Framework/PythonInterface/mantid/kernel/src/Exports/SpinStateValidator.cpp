// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SpinStateValidator.h"
#include "MantidKernel/TypedValidator.h"
#include "MantidPythonInterface/core/TypedValidatorExporter.h"

#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::SpinStateValidator;
using Mantid::Kernel::TypedValidator;
using Mantid::PythonInterface::TypedValidatorExporter;
using namespace boost::python;

std::shared_ptr<SpinStateValidator>
createSpinStateValidator(list allowedNumberOfSpins, const bool acceptSingleStates = false,
                         const std::string &paraIndicator = "0", const std::string &antiIndicator = "1",
                         const bool optional = false, const std::string &extraIndicator = "") {

  std::unordered_set<int> allowedNumberOfSpinsSet;
  for (int i = 0; i < len(allowedNumberOfSpins); i++) {
    int spinN = extract<int>(allowedNumberOfSpins[i]);
    allowedNumberOfSpinsSet.insert(spinN);
  }

  return std::make_shared<SpinStateValidator>(allowedNumberOfSpinsSet, acceptSingleStates, paraIndicator, antiIndicator,
                                              optional, extraIndicator);
}

void export_SpinStateValidator() {
  TypedValidatorExporter<std::string>::define("StringTypedValidator");

  class_<SpinStateValidator, bases<TypedValidator<std::string>>, std::shared_ptr<SpinStateValidator>,
         boost::noncopyable>("SpinStateValidator", no_init)
      .def("__init__",
           make_constructor(&createSpinStateValidator, default_call_policies(),
                            (arg("allowedNumberOfSpins"), arg("acceptSingleStates") = false, arg("paraIndicator") = "0",
                             arg("antiIndicator") = "1", arg("optional") = false, arg("extraIndicator") = "")),
           "Will check that a string matches the form 01,00 or 00,10,11,01, for example. This is used for specifying "
           "the order of input workspaces relative to spin states.");
}
