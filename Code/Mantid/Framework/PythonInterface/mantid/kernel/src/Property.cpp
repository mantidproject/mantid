#include "MantidKernel/Property.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/make_function.hpp>

using Mantid::Kernel::Property;
using Mantid::Kernel::Direction;
using namespace boost::python;


void export_Property()
{
  // Ptr<->Object conversion
  register_ptr_to_python<Property*>();

  //Direction
  enum_<Direction::Type>("Direction")
    .value("Input", Direction::Input)
    .value("Output", Direction::Output)
    .value("InOut", Direction::InOut)
    .value("None", Direction::None)
    ;

  class_<Property, boost::noncopyable>("Property", no_init)
    .add_property("name", make_function(&Mantid::Kernel::Property::name, return_value_policy<copy_const_reference>()), "The name of the property")
    .add_property("value", &Mantid::Kernel::Property::value, "The value of the property as a string")
    .add_property("is_valid", &Mantid::Kernel::Property::isValid, "An empty string if the property is valid, otherwise it contains an error message.")
    .add_property("allowed_values", &Mantid::Kernel::Property::allowedValues, "A list of allowed values")
    .add_property("direction", &Mantid::Kernel::Property::direction, "Input, Output, InOut or Unknown. See the Direction enum")
    .add_property("units", &Mantid::Kernel::Property::units, "The units attached to this property")
    .add_property("is_default", &Mantid::Kernel::Property::isDefault, "Is the property set at the default value")
   ;
}

/**
 * I see know way around explicitly declaring each we use
 */
void export_PropertyWithValue()
{
  EXPORT_PROP_W_VALUE(int, _int);
  EXPORT_PROP_W_VALUE(double, _dbl);
  EXPORT_PROP_W_VALUE(bool, _bool);
  EXPORT_PROP_W_VALUE(std::string, _str);
}
