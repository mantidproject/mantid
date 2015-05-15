#include "MantidAPI/FunctionProperty.h"
#include "MantidPythonInterface/kernel/PropertyWithValueExporter.h"
#include <boost/python/class.hpp>

using Mantid::API::FunctionProperty;
using Mantid::API::IFunction;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::PropertyWithValueExporter;
using namespace boost::python;

// clang-format off
void export_FunctionProperty()
// clang-format on
{
  // FuncitonProperty has base PropertyWithValue<boost::shared_ptr<IFunction>>
  // which must be exported
  typedef boost::shared_ptr<IFunction> HeldType;
  PropertyWithValueExporter<HeldType>::define("FunctionPropertyWithValue");


  class_<FunctionProperty, bases<PropertyWithValue<HeldType>>, boost::noncopyable>("FunctionProperty", no_init)
    .def(init<const std::string &>(arg("name"), "Constructs a FunctionProperty with the given name"))
    ;
}

