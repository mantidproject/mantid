#include "MantidKernel/VisibleWhenProperty.h"
#include <memory>

namespace Mantid {
namespace Kernel {
VisibleWhenProperty::VisibleWhenProperty(std::string otherPropName,
                                         ePropertyCriterion when,
                                         std::string value)
    : EnabledWhenProperty(otherPropName, when, value) {}

VisibleWhenProperty::VisibleWhenProperty(
    std::unique_ptr<VisibleWhenProperty> &&conditionOne,
    std::unique_ptr<VisibleWhenProperty> &&conditionTwo,
    eLogicOperator logicalOperator)
    : EnabledWhenProperty(std::move(conditionOne), std::move(conditionTwo),
                          logicalOperator) {}

bool VisibleWhenProperty::isEnabled(const IPropertyManager *) const {
  return true;
}

bool VisibleWhenProperty::isVisible(const IPropertyManager *algo) const {
  if (m_propertyDetails) {
    return checkCriterion(algo);
  } else if (m_comparisonDetails) {
    return checkComparison(algo);
  } else {
    throw std::runtime_error("Both PropertyDetails and ComparisonDetails were "
                             "null in VisibleWhenProperty");
  }
}

bool VisibleWhenProperty::checkComparison(const IPropertyManager *algo) const {
  const auto &comparison = m_comparisonDetails;
  const auto &objectOne = comparison->conditionOne;
  const auto &objectTwo = comparison->conditionTwo;

  switch (comparison->logicOperator) {
  case AND:
    return objectOne->isVisible(algo) && objectTwo->isVisible(algo);
    break;
  case OR:
    return objectOne->isVisible(algo) || objectTwo->isVisible(algo);
    break;
  case XOR:
    return objectOne->isVisible(algo) ^ objectTwo->isVisible(algo);
    break;
  default:
    throw std::runtime_error("Unknown logic operator in EnabledWhenProperty");
  }
}

IPropertySettings *VisibleWhenProperty::clone() {
  VisibleWhenProperty *out = new VisibleWhenProperty(
      m_propertyDetails->otherPropName, m_propertyDetails->criterion,
      m_propertyDetails->value);
  return out;
}

} // namespace Mantid
} // namespace Kernel
