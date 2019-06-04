// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IValidator.h"
#include "MantidPythonInterface/kernel/PropertyWithValueExporter.h"
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/make_constructor.hpp>

using namespace boost::python;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;

void export_OptionalBoolValue() {
  boost::python::enum_<OptionalBool::Value>("OptionalBoolValue")
      .value(OptionalBool::StrUnset.c_str(), OptionalBool::Unset)
      .value(OptionalBool::StrTrue.c_str(), OptionalBool::True)
      .value(OptionalBool::StrFalse.c_str(), OptionalBool::False)
      // Python 3 does not allow .True/.False. It gives a syntax error
      // so use an underscore after the name
      .value((OptionalBool::StrTrue + "_").c_str(), OptionalBool::True)
      .value((OptionalBool::StrFalse + "_").c_str(), OptionalBool::False);
}

namespace {
OptionalBool *createOptionalBool(OptionalBool::Value value) {
  auto state = OptionalBool::Value(value);
  return new OptionalBool(state);
}

class OptionalBoolPropertyValueHandler : public Registry::PropertyValueHandler {

private:
  OptionalBool fromPyObj(const boost::python::object &value) const {
    OptionalBool target;
    const extract<OptionalBool> asDirect(value);
    const extract<OptionalBool::Value> asEnum(value);
    const extract<bool> asBool(value);

    if (asDirect.check()) {
      target = asDirect();
    } else if (asEnum.check()) {
      target = OptionalBool(asEnum());
    } else if (asBool.check()) {
      target = OptionalBool(asBool());
    } else {
      throw std::invalid_argument("Unknown conversion to OptionalBool");
    }
    return target;
  }

public:
  using HeldType = OptionalBool;

  /**
   * Set function to handle Python -> C++ calls and get the correct type
   */
  void set(Mantid::Kernel::IPropertyManager *alg, const std::string &name,
           const boost::python::object &value) const override {

    alg->setProperty<OptionalBool>(name, fromPyObj(value));
  }

  /**
   * Create a PropertyWithValue from the given python object value
   */
  std::unique_ptr<Mantid::Kernel::Property>
  create(const std::string &name, const boost::python::object &value,
         const boost::python::object &validator,
         const unsigned int direction) const override {
    using boost::python::extract;

    auto optBool = fromPyObj(value);
    if (isNone(validator)) {
      return std::make_unique<PropertyWithValue<OptionalBool>>(
          name, optBool, direction);
    } else {
      const IValidator *propValidator = extract<IValidator *>(validator);
      return std::make_unique<PropertyWithValue<OptionalBool>>(
          name, optBool, propValidator->clone(), direction);
    }
  }
};
} // namespace

void export_OptionalBool() {
  // V3D class
  class_<OptionalBool>("OptionalBool")
      .def("__init__",
           make_constructor(&createOptionalBool, default_call_policies(),
                            (arg("value"))))
      .def("getValue", &OptionalBool::getValue, arg("self"));
}

void export_PropertyWithValueOptionalBool() {
  using Mantid::PythonInterface::PropertyWithValueExporter;
  // ints & vectors
  PropertyWithValueExporter<OptionalBool>::define(
      "OptionalBoolPropertyWithValue");

  Registry::TypeRegistry::subscribe<OptionalBoolPropertyValueHandler>();
}
