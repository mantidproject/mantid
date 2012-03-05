#include "MantidKernel/Property.h"
#include "MantidPythonInterface/kernel/StlExportDefinitions.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/make_function.hpp>

using Mantid::Kernel::Property;
using Mantid::Kernel::Direction;
using Mantid::PythonInterface::std_vector_exporter;
using namespace boost::python;


void export_Property()
{
  register_ptr_to_python<Property*>();

  // vector of properties
  std_vector_exporter<Property*>::wrap("std_vector_property");

  //Direction
  enum_<Direction::Type>("Direction")
    .value("Input", Direction::Input)
    .value("Output", Direction::Output)
    .value("InOut", Direction::InOut)
    .value("None", Direction::None)
    ;

  class_<Property, boost::noncopyable>("Property", no_init)
    .add_property("name", make_function(&Mantid::Kernel::Property::name, return_value_policy<copy_const_reference>()),
                  "The name of the property")
    .add_property("documentation", make_function(&Mantid::Kernel::Property::documentation, return_value_policy<copy_const_reference>()),
                  "The property's doc string")
    .add_property("direction", &Mantid::Kernel::Property::direction, "Input, Output, InOut or Unknown. See the Direction enum")
    .add_property("units", &Mantid::Kernel::Property::units, "The units attached to this property")
    .add_property("valueAsStr", &Mantid::Kernel::Property::value, "The value of the property as a string. "
        "For some property types, e.g. Workspaces, it is useful to be able to refer to the string value directly")
    .def("isValid", &Mantid::Kernel::Property::isValid, "An empty string if the property is valid, otherwise it contains an error message.")
    .def("allowedValues", &Mantid::Kernel::Property::allowedValues, "A list of allowed values")
    .def("isDefault", &Mantid::Kernel::Property::isDefault, "Is the property set at the default value")
    .def("getGroup", &Mantid::Kernel::Property::getGroup, return_value_policy<copy_const_reference>(),
          "Return the 'group' of the property, that is, the header in the algorithm's list of properties.")
   ;
}
