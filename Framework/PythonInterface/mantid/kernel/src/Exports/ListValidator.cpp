#include "MantidKernel/ListValidator.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/default_call_policies.hpp>

#include <string>

using Mantid::Kernel::ListValidator;
using Mantid::Kernel::IValidator;
namespace Converters = Mantid::PythonInterface::Converters;
using namespace boost::python;

namespace {

/**
  * Factory function to allow the allowed values to be specified as a python
 * list
  * @param allowedValues :: The list of allowed values
  * @return A new ListValidator instance
  */
template <typename T>
ListValidator<T> *
createListValidator(const boost::python::list &allowedValues) {
  return new ListValidator<T>(
      Converters::PySequenceToVector<T>(allowedValues)());
}

#define EXPORT_LISTVALIDATOR(type, prefix)                                     \
  class_<ListValidator<type>, bases<IValidator>, boost::noncopyable>(          \
      #prefix "ListValidator")                                                 \
      .def("__init__",                                                         \
           make_constructor(&createListValidator<type>,                        \
                            default_call_policies(), arg("allowedValues")))    \
      .def("addAllowedValue", &ListValidator<type>::addAllowedValue,           \
           "Adds a value to the list of accepted values");
}

void export_ListValidator() {
  EXPORT_LISTVALIDATOR(std::string, String);
  EXPORT_LISTVALIDATOR(long, Int);
}
