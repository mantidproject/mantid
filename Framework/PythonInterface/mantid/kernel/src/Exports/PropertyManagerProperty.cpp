#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyManager.h"

#include "MantidPythonInterface/kernel/PropertyWithValueExporter.h"
#include <boost/python/class.hpp>

using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyManagerProperty;
using Mantid::Kernel::PropertyManager_sptr;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::PropertyWithValueExporter;
using namespace boost::python;

void export_PropertyManagerProperty() {
  // export base class
  typedef PropertyManager_sptr BaseValueType;
  PropertyWithValueExporter<BaseValueType>::define(
      "PropertyManagerPropertyWithValue");

  // leaf class type
  typedef PropertyManagerProperty::BaseClass BaseClassType;
  class_<PropertyManagerProperty, bases<BaseClassType>, boost::noncopyable>(
      "PropertyManagerProperty", no_init)
      .def(init<const std::string &, const unsigned int>(
          (arg("self"), arg("name"), arg("direction") = Direction::Input),
          "Construct an PropertyManagerProperty"));
}
