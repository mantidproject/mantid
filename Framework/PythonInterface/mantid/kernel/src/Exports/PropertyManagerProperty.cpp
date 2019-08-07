// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyManager.h"

#include "MantidPythonInterface/core/PropertyWithValueExporter.h"
#include "MantidPythonInterface/kernel/Registry/MappingTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/PropertyManagerFactory.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>

#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyManagerProperty;
using Mantid::Kernel::PropertyManager_sptr;
using Mantid::PythonInterface::PropertyWithValueExporter;
namespace Registry = Mantid::PythonInterface::Registry;
using namespace boost::python;

PropertyManagerProperty *
createPropertyManagerPropertyWithDict(const std::string &name,
                                      const boost::python::dict &value,
                                      const unsigned int &direction) {
  return new PropertyManagerProperty(
      name, Registry::createPropertyManager(value), direction);
}

void export_PropertyManagerProperty() {
  // export base class
  using BaseValueType = PropertyManager_sptr;
  PropertyWithValueExporter<BaseValueType>::define(
      "PropertyManagerPropertyWithValue");

  // leaf class type
  using BaseClassType = PropertyManagerProperty::BaseClass;
  class_<PropertyManagerProperty, bases<BaseClassType>, boost::noncopyable>(
      "PropertyManagerProperty", no_init)
      .def(init<const std::string &, const unsigned int>(
          (arg("self"), arg("name"), arg("direction") = Direction::Input),
          "Construct an PropertyManagerProperty"))
      .def("__init__", make_constructor(&createPropertyManagerPropertyWithDict,
                                        default_call_policies(),
                                        (arg("name"), arg("value"),
                                         arg("direction") = Direction::Input)));

  // type handler for alg.setProperty calls
  Registry::TypeRegistry::subscribe(typeid(BaseValueType),
                                    new Registry::MappingTypeHandler);
}
