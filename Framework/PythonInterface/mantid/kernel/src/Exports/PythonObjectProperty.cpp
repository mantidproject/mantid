// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/PythonObjectProperty.h"

#include "MantidPythonInterface/core/PropertyWithValueExporter.h"
#include "MantidPythonInterface/kernel/Registry/PythonObjectTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"

#ifndef Q_MOC_RUN
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#endif

using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::PropertyWithValueExporter;
using Mantid::PythonInterface::PythonObjectProperty;
namespace Registry = Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_PythonObjectProperty() {
  // export base class
  using BaseValueType = boost::python::object;
  PropertyWithValueExporter<BaseValueType>::define("PythonObjectPropertyWithValue");

  // leaf class type
  using BaseClassType = PythonObjectProperty::BaseClass;
  class_<PythonObjectProperty, bases<BaseClassType>, boost::noncopyable>("PythonObjectProperty", no_init)
      .def(init<const std::string &, const unsigned int>(
          (arg("self"), arg("name"), arg("direction") = Direction::Input), "Construct a PythonObjectProperty"))
      .def(init<const std::string &, const boost::python::object &, const unsigned int>(
          (arg("self"), arg("name"), arg("defaultValue"), arg("direction") = Direction::Input),
          "Construct a PythonObjectProperty with a default value"))
      .add_property("value", make_function(&PythonObjectProperty::operator(),
                                           return_value_policy<boost::python::return_by_value>()));

  // type handler for alg.setProperty calls
  Registry::TypeRegistry::subscribe(typeid(BaseValueType), new Registry::PythonObjectTypeHandler);
}
