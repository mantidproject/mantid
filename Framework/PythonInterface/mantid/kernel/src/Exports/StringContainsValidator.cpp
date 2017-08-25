#include "MantidKernel/StringContainsValidator.h"

#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::StringContainsValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {
StringContainsValidator *createStringContainsValidator() {
  return new StringContainsValidator();
}
}

void export_StringContainsValidator() {
  class_<StringContainsValidator, bases<IValidator>, boost::noncopyable>(
      "StringContainsValidator")
      .def("__init__", make_constructor(&createStringContainsValidator,
                                        default_call_policies()))
      .def("setRequiredStrings", &StringContainsValidator::setRequiredStrings,
           "Set the list of sub strings that the input must contain");
}

