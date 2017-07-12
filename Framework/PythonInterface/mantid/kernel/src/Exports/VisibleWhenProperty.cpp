#include "MantidKernel/VisibleWhenProperty.h"
#include <boost/python/class.hpp>

using namespace Mantid::Kernel;
using namespace boost::python;

void export_VisibleWhenProperty() {
  class_<VisibleWhenProperty, bases<EnabledWhenProperty>>("VisibleWhenProperty",
                                                          no_init)
      .def(init<std::string, ePropertyCriterion, std::string>(
          (arg("self"), arg("otherPropName"), arg("when"), arg("value")),
          "Enabled otherPropName property when value criterion meets that "
          "given by the 'when' argument"))

      .def(init<std::string, ePropertyCriterion>(
          (arg("self"), arg("otherPropName"), arg("when")),
          "Enabled otherPropName property when criterion does not require a "
          "value, i.e isDefault"))

      .def(init<VisibleWhenProperty, VisibleWhenProperty, eLogicOperator>(
          (arg("self"), arg("conditionObjOne"), arg("conditionObjTwo")),
          "Sets the property to visible when the two VisibleWhenProperty"
          " conditions match the operator condition"));
}
