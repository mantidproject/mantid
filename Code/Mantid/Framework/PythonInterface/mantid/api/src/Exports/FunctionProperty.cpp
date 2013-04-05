#include "MantidAPI/FunctionProperty.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include <boost/python/class.hpp>

using Mantid::API::FunctionProperty;
using Mantid::API::IFunction;
using Mantid::Kernel::PropertyWithValue;
using namespace boost::python;

void export_FunctionProperty()
{
  // FuncitonProperty has base PropertyWithValue<boost::shared_ptr<IFunction>>
  // which must be exported
  typedef boost::shared_ptr<IFunction> HeldType;
  EXPORT_PROP_W_VALUE(HeldType, IFunction);

  class_<FunctionProperty, bases<PropertyWithValue<HeldType>>, boost::noncopyable>("FunctionProperty", no_init)
    .def(init<const std::string &>(arg("name"), "Constructs a FunctionProperty with the given name"))
    ;
}

