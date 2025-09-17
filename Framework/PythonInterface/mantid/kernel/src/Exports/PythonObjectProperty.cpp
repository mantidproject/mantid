// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/PythonObjectProperty.h"
#include "MantidKernel/NullValidator.h"

#include "MantidPythonInterface/core/PropertyWithValueExporter.h"
#include "MantidPythonInterface/kernel/Registry/PythonObjectTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"

#ifndef Q_MOC_RUN
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#endif

#include <boost/python/default_call_policies.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::Kernel::Direction;
using Mantid::Kernel::IValidator_sptr;
using Mantid::Kernel::NullValidator;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::PropertyWithValueExporter;
using Mantid::PythonInterface::PythonObjectProperty;
namespace Registry = Mantid::PythonInterface::Registry;
using namespace boost::python;

namespace {

PythonObjectProperty *createPythonObjectProperty(std::string const &name, boost::python::object const &value,
                                                 IValidator_sptr const &validator, unsigned int const direction) {
  return new PythonObjectProperty(name, value, validator, direction);
}
} // namespace

void export_PythonObjectProperty() {
  // export base class
  using BaseValueType = boost::python::object;
  PropertyWithValueExporter<BaseValueType>::define("PythonObjectPropertyWithValue");

  // leaf class type
  using BaseClassType = PythonObjectProperty::BaseClass;
  class_<PythonObjectProperty, bases<BaseClassType>, boost::noncopyable>("PythonObjectProperty", no_init)
      // name and direction
      .def(init<const std::string &, const unsigned int>(
          (arg("self"), arg("name"), arg("direction") = Direction::Input), "Construct a PythonObjectProperty"))

      // name, validator, and direction
      .def(init<const std::string &, IValidator_sptr, const unsigned int>(
          (arg("self"), arg("name"), arg("validator"), arg("direction") = Direction::Input),
          "Construct a PythonObjectProperty with a validator"))

      // name, default, and direction
      .def(init<const std::string &, const boost::python::object &, const unsigned int>(
          (arg("self"), arg("name"), arg("defaultValue"), arg("direction") = Direction::Input),
          "Construct a PythonObjectProperty with a default value"))

      // name, default, validator, and direction
      .def(init<const std::string &, const boost::python::object &, IValidator_sptr, const unsigned int>(
          (arg("self"), arg("name"), arg("defaultValue"), arg("validator") = std::make_shared<NullValidator>(),
           arg("direction") = Direction::Input),
          "Construct a PythonObjectProperty with a default value and validator"))

      .def("__init__",
           make_constructor(&createPythonObjectProperty, default_call_policies(),
                            (arg("name"), arg("value"), arg("validator") = IValidator_sptr(new NullValidator),
                             arg("direction") = Direction::Input)))

      .add_property("value", make_function(&PythonObjectProperty::operator(),
                                           return_value_policy<boost::python::return_by_value>()))
      .def("setValue", static_cast<std::string (PythonObjectProperty::*)(boost::python::object const &)>(
                           &PythonObjectProperty::setValue));

  // type handler for alg.setProperty calls
  Registry::TypeRegistry::subscribe(typeid(BaseValueType), new Registry::PythonObjectTypeHandler);
}
