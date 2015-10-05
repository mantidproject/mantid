#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/kernel/PropertyWithValueExporter.h"
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
AlgorithmProperty *createPropertyWithValidatorAndDirection(
    const std::string &name, IValidator *validator, unsigned int direction) {
  return new AlgorithmProperty(name, validator->clone(), direction);
}

/**
 * Factory function for creating an input property with a validator
 * @param name The name of the property
 * @param validator A pointer to the validator passed from Python. It is cloned
 * when passed to the framework
 * @return A pointer to a new AlgorithmProperty object
 */
AlgorithmProperty *createPropertyWithValidator(const std::string &name,
                                               IValidator *validator) {
  return createPropertyWithValidatorAndDirection(
      name, validator, Mantid::Kernel::Direction::Input);
}
}

void export_AlgorithmProperty() {
  // AlgorithmProperty has base PropertyWithValue<boost::shared_ptr<IAlgorithm>>
  // which must be exported
  typedef boost::shared_ptr<IAlgorithm> HeldType;
  PropertyWithValueExporter<HeldType>::define("AlgorithmPropertyWithValue");

  class_<AlgorithmProperty, bases<PropertyWithValue<HeldType>>,
         boost::noncopyable>("AlgorithmProperty", no_init)
      .def(init<const std::string &>(args("name")))
      // These variants require the validator object to be cloned
      .def("__init__",
           make_constructor(&createPropertyWithValidator,
                            default_call_policies(), args("name", "validator")))
      .def("__init__",
           make_constructor(&createPropertyWithValidatorAndDirection,
                            default_call_policies(),
                            args("name", "validator", "direction")));
}
