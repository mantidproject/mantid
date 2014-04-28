#include "MantidKernel/EnabledWhenProperty.h"
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>

using namespace Mantid::Kernel;
using namespace boost::python;

void export_EnabledWhenProperty()
{
  // State enumeration
  enum_<ePropertyCriterion>("PropertyCriterion")
    .value("IsDefault", IS_DEFAULT)
    .value("IsNotDefault", IS_NOT_DEFAULT)
    .value("IsEqualTo", IS_EQUAL_TO)
    .value("IsNotEqualTo", IS_NOT_EQUAL_TO)
    .value("IsMoreOrEqual", IS_MORE_OR_EQ)
  ;

  class_<EnabledWhenProperty, bases<IPropertySettings>,
         boost::noncopyable>("EnabledWhenProperty", no_init) // no default constructor

     .def(init<std::string, ePropertyCriterion, std::string>(
             (arg("otherPropName"), arg("when"), arg("value")),
              "Enabled otherPropName property when value criterion meets that given by the 'when' argument")
         )

     .def(init<std::string, ePropertyCriterion>(
              (arg("otherPropName"), arg("when")),
              "Enabled otherPropName property when criterion does not require a value, i.e isDefault")
             )
    ;
}

