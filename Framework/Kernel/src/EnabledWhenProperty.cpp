#include "MantidKernel/EnabledWhenProperty.h"

#include <boost/lexical_cast.hpp>
#include <exception>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {
EnabledWhenProperty::EnabledWhenProperty(const std::string &otherPropName,
                                         const ePropertyCriterion when,
                                         const std::string &value)
    : IPropertySettings() {
  m_propertyDetails =
      std::make_unique<PropertyDetails>(otherPropName, when, value);
}

EnabledWhenProperty::EnabledWhenProperty(
    const EnabledWhenProperty conditionOne,
    const EnabledWhenProperty conditionTwo,
    const eComparisonCriterion logicalOperator)
    : IPropertySettings() {
  m_comparisonDetails = std::make_unique<ComparisonDetails>(
      conditionOne, conditionTwo, logicalOperator);
}

bool EnabledWhenProperty::fulfillsCriterion(
    const IPropertyManager *algo) const {
  // Find the property
  if (algo == nullptr)
    return true;
  Property *prop = nullptr;
  try {
    prop = algo->getPointerToProperty(m_propertyDetails->otherPropName);
  } catch (Exception::NotFoundError &) {
    return true; // Property not found. Ignore
  }
  if (!prop)
    return true;

  // Value of the other property
  const std::string propValue = prop->value();

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

bool EnabledWhenProperty::isEnabled(const IPropertyManager *algo) const {
  return fulfillsCriterion(algo);
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
