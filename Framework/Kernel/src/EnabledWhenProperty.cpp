#include "MantidKernel/EnabledWhenProperty.h"

#include <boost/lexical_cast.hpp>
#include <exception>
#include <memory>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {

using EnabledPropPtr = std::unique_ptr<EnabledWhenProperty>;

EnabledWhenProperty::EnabledWhenProperty(const std::string &otherPropName,
                                         const ePropertyCriterion when,
                                         const std::string &value)
    : IPropertySettings() {
  m_propertyDetails =
      std::make_unique<PropertyDetails>(otherPropName, when, value);
}

EnabledWhenProperty::EnabledWhenProperty(EnabledPropPtr &&conditionOne,
                                         EnabledPropPtr &&conditionTwo,
                                         eLogicOperator logicalOperator)
    : IPropertySettings() {
  m_comparisonDetails = std::make_unique<ComparisonDetails>(
      std::move(conditionOne), std::move(conditionTwo), logicalOperator);
}

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

bool EnabledWhenProperty::isVisible(const IPropertyManager *) const {
  return true;
}
/// does nothing in this case and put here to satisfy the interface.
void EnabledWhenProperty::modify_allowed_values(Property *const) {}
//--------------------------------------------------------------------------------------------
/// Make a copy of the present type of validator
IPropertySettings *EnabledWhenProperty::clone() {
  const PropertyDetails &propDetails = *m_propertyDetails;
  EnabledWhenProperty *out = new EnabledWhenProperty(
      propDetails.otherPropName, propDetails.criterion, propDetails.value);
  return out;
}

} // namespace Mantid
} // namespace Kernel
