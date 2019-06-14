// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/EnabledWhenProperty.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Property.h"

#include <boost/lexical_cast.hpp>

#include <exception>
#include <memory>
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {
/** Constructor
 * @param otherPropName :: Name of the OTHER property that we will check.
 * @param when :: Criterion to evaluate
 * @param value :: For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value
 * (as string) to check for
 */
EnabledWhenProperty::EnabledWhenProperty(const std::string &otherPropName,
                                         const ePropertyCriterion when,
                                         const std::string &value)
    : IPropertySettings() {
  m_propertyDetails = std::make_shared<PropertyDetails>(
      PropertyDetails{otherPropName, when, value});
}

/** Multiple conditions constructor - takes two enable when property
 * objects and returns the product of of the with the specified
 * logic operator
 *
 * @param conditionOne :: First EnabledWhenProperty object to use
 * @param conditionTwo :: Second EnabledWhenProperty object to use
 * @param logicOperator :: The logic operator to apply across both
 * conditions
 */
EnabledWhenProperty::EnabledWhenProperty(
    const EnabledWhenProperty &conditionOne,
    const EnabledWhenProperty &conditionTwo, eLogicOperator logicOperator)
    : // This method allows the Python interface to easily construct these
      // objects
      // Copy the object then forward onto our move constructor
      EnabledWhenProperty(std::make_shared<EnabledWhenProperty>(conditionOne),
                          std::make_shared<EnabledWhenProperty>(conditionTwo),
                          logicOperator) {}

/** Multiple conditions constructor - takes two shared pointers to
 * EnabledWhenProperty objects and returns the product of them
 * with the specified logic operator.
 *
 * @param conditionOne :: First EnabledWhenProperty object to use
 * @param conditionTwo :: Second EnabledWhenProperty object to use
 * @param logicOperator :: The logic operator to apply across both
 *conditions
 *
 */
EnabledWhenProperty::EnabledWhenProperty(
    std::shared_ptr<EnabledWhenProperty> &&conditionOne,
    std::shared_ptr<EnabledWhenProperty> &&conditionTwo,
    eLogicOperator logicOperator)
    : IPropertySettings() {
  // Initialise with POD compatible syntax
  m_comparisonDetails =
      std::make_shared<ComparisonDetails<EnabledWhenProperty>>(
          ComparisonDetails<EnabledWhenProperty>{
              std::move(conditionOne), std::move(conditionTwo), logicOperator});
}

EnabledWhenProperty::EnabledWhenProperty(const EnabledWhenProperty &other)
    : IPropertySettings(), m_propertyDetails{other.m_propertyDetails},
      m_comparisonDetails{other.m_comparisonDetails} {}

/**
 * Checks if the user specified combination of enabled criterion
 * returns a true or false value
 *
 * @param algo :: The algorithm containing the property to check
 * @return :: true if user specified combination was true, else false.
 * @throw ::  If any problems was found
 */
bool EnabledWhenProperty::checkComparison(const IPropertyManager *algo) const {
  const auto &comparison = m_comparisonDetails;
  const auto &objectOne = comparison->conditionOne;
  const auto &objectTwo = comparison->conditionTwo;

  switch (comparison->logicOperator) {
  case AND:
    return objectOne->isEnabled(algo) && objectTwo->isEnabled(algo);
    break;
  case OR:
    return objectOne->isEnabled(algo) || objectTwo->isEnabled(algo);
    break;
  case XOR:
    return objectOne->isEnabled(algo) ^ objectTwo->isEnabled(algo);
    break;
  default:
    throw std::runtime_error("Unknown logic operator in EnabledWhenProperty");
  }
}

/**
 * Does the validator fulfill the criterion based on the
 * other property values?
 * @param algo :: The pointer to the algorithm to check the property values of
 * @return :: True if the criteria are met else false
 * @throw :: Throws on any problems (e.g. property missing from algorithm)
 */
bool EnabledWhenProperty::checkCriterion(const IPropertyManager *algo) const {

  // Value of the other property
  const std::string propValue = getPropertyValue(algo);
  // This is safe as long as getPropertyValue (which checks) has been called
  // already
  auto prop = algo->getPointerToProperty(m_propertyDetails->otherPropName);

  // OK, we have the property. Check the condition
  switch (m_propertyDetails->criterion) {
  case IS_DEFAULT:
    return prop->isDefault();
  case IS_NOT_DEFAULT:
    return !prop->isDefault();
  case IS_EQUAL_TO:
    return (propValue == m_propertyDetails->value);
  case IS_NOT_EQUAL_TO:
    return (propValue != m_propertyDetails->value);
  case IS_MORE_OR_EQ: {
    int check = boost::lexical_cast<int>(m_propertyDetails->value);
    int iPropV = boost::lexical_cast<int>(propValue);
    return (iPropV >= check);
  }
  default:
    // Unknown criterion
    std::string errString = "The EnabledWhenProperty criterion set"
                            " for the following property ";
    errString += m_propertyDetails->otherPropName;
    errString += " is unknown";
    throw std::invalid_argument(errString);
  }
}

/**
 * Checks the algorithm given is in a valid state and the property
 * exists then proceeds to try to get the value associated
 *
 * @param algo :: The pointer to the algorithm to process
 * @return :: The value contained by said property
 * @throw :: Throws if anything is wrong with the property or algorithm
 */
std::string
EnabledWhenProperty::getPropertyValue(const IPropertyManager *algo) const {
  // Find the property
  if (algo == nullptr)
    throw std::runtime_error(
        "Algorithm properties passed to EnabledWhenProperty was null");

  Property *prop = nullptr;
  try {
    prop = algo->getPointerToProperty(m_propertyDetails->otherPropName);
  } catch (Exception::NotFoundError &) {
    prop = nullptr; // Ensure we still have null pointer
  }
  if (!prop)
    throw std::runtime_error("Property " + m_propertyDetails->otherPropName +
                             " was not found in EnabledWhenProperty");
  return prop->value();
}

/**
 * Returns whether the property should be enabled or disabled based
 * on the property conditions
 *
 * @param algo :: The algorithm containing the property
 * @return :: True if enabled when conditions matched, else false
 * @throw :: Throws on any error (e.g. missing property)
 */
bool EnabledWhenProperty::isEnabled(const IPropertyManager *algo) const {
  if (m_propertyDetails) {
    return checkCriterion(algo);
  } else if (m_comparisonDetails) {
    return checkComparison(algo);
  } else {
    throw std::runtime_error("Both PropertyDetails and ComparisonDetails were "
                             "null in EnabledWhenProperty");
  }
}

/**
 * Always returns true as EnabledWhenProperty always sets the visibility
 * on while altering whether the property is Enabled or disabled
 *
 * @param algo :: Pointer to the algorithm containing the property
 * @return :: True always
 */
bool EnabledWhenProperty::isVisible(const IPropertyManager *algo) const {
  // VisisbleWhenProperty uses algo so we have to keep it to match interface
  UNUSED_ARG(algo);
  return true;
}

/// Does nothing in this case and put here to satisfy the interface.
void EnabledWhenProperty::modify_allowed_values(Property *const /*unused*/) {}

/**
 * Clones the current EnabledWhenProperty object and returns
 * a pointer to the new object. The caller is responsible
 * for deleting this pointer when finished
 *
 * @return Pointer to cloned EnabledWhenProperty object
 */
IPropertySettings *EnabledWhenProperty::clone() const {
  return new EnabledWhenProperty(*this);
}

} // namespace Kernel
} // namespace Mantid
