// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/core/PropertyWithValueExporter.h"
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>

using Mantid::API::AlgorithmProperty;
using Mantid::API::IAlgorithm;
using Mantid::Kernel::IValidator;
using Mantid::Kernel::PropertyWithValue;
using Mantid::PythonInterface::PropertyWithValueExporter;
using namespace boost::python;

namespace {
/**
 * Factory function for creating an input property with a validator and a
 * direction
 * @param name The name of the property
 * @param validator A pointer to the validator passed from Python. It is cloned
 * when passed to the framework
 * @param direction An output/input/inout property
 * @return A pointer to a new AlgorithmProperty object
 */
AlgorithmProperty *createPropertyWithValidatorAndDirection(const std::string &name, const IValidator *validator,
                                                           unsigned int direction) {
  return new AlgorithmProperty(name, validator->clone(), direction);
}

/**
 * Factory function for creating an input property with a validator
 * @param name The name of the property
 * @param validator A pointer to the validator passed from Python. It is cloned
 * when passed to the framework
 * @return A pointer to a new AlgorithmProperty object
 */
const std::function<const AlgorithmProperty *(const std::string &, const IValidator *)> createPropertyWithValidator =
    [](const std::string &name, const IValidator *validator) {
      return createPropertyWithValidatorAndDirection(name, validator, Mantid::Kernel::Direction::Input);
    };

} // namespace

void export_AlgorithmProperty() {
  // AlgorithmProperty has base PropertyWithValue<std::shared_ptr<IAlgorithm>>
  // which must be exported
  using HeldType = std::shared_ptr<IAlgorithm>;
  PropertyWithValueExporter<HeldType>::define("AlgorithmPropertyWithValue");

  class_<AlgorithmProperty, bases<PropertyWithValue<HeldType>>, boost::noncopyable>("AlgorithmProperty", no_init)
      .def(init<const std::string &>(args("name")))
      // These variants require the validator object to be cloned
      .def("__init__",
           make_constructor(&createPropertyWithValidator, default_call_policies(), args("name", "validator")))
      .def("__init__", make_constructor(&createPropertyWithValidatorAndDirection, default_call_policies(),
                                        args("name", "validator", "direction")));
}
