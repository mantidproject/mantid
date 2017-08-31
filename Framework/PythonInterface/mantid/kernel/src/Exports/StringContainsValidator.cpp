#include "MantidKernel/StringContainsValidator.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"

#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/list.hpp>

using Mantid::Kernel::StringContainsValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {
  StringContainsValidator *createStringContainsValidator() {
    return new StringContainsValidator();
  }

/// Set required strings from a python list
void setRequiredStrings(StringContainsValidator &self,
          const boost::python::list &strs) {
    using namespace Mantid::PythonInterface;
    self.setRequiredStrings(Converters::PySequenceToVector<std::string>(strs)());

}
}

void export_StringContainsValidator() {
  class_<StringContainsValidator, bases<IValidator>, boost::noncopyable>(
      "StringContainsValidator")
      .def("__init__", make_constructor(&createStringContainsValidator,
                                        default_call_policies()))
      .def("setRequiredStrings", &setRequiredStrings,
           arg("Strings"),
           "Set the list of sub strings that the input must contain");
}

