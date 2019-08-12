// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/StringContainsValidator.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"

#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::IValidator;
using Mantid::Kernel::StringContainsValidator;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

namespace {
StringContainsValidator *createStringContainsValidator() {
  return new StringContainsValidator();
}

StringContainsValidator *
createStringContainsValidatorWithStrings(const boost::python::list &values) {
  return new StringContainsValidator(
      Converters::PySequenceToVector<std::string>(values)());
}

/// Set required strings from a python list
void setRequiredStrings(StringContainsValidator &self,
                        const boost::python::list &strs) {
  self.setRequiredStrings(Converters::PySequenceToVector<std::string>(strs)());
}
} // namespace

void export_StringContainsValidator() {
  class_<StringContainsValidator, bases<IValidator>, boost::noncopyable>(
      "StringContainsValidator")
      .def("__init__", make_constructor(&createStringContainsValidator,
                                        default_call_policies()))
      .def("__init__",
           make_constructor(&createStringContainsValidatorWithStrings,
                            default_call_policies(), (arg("values"))))
      .def("setRequiredStrings", &setRequiredStrings, arg("Strings"),
           "Set the list of sub strings that the input must contain");
}
