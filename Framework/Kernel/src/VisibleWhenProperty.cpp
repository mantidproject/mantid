#include "MantidKernel/VisibleWhenProperty.h"

namespace Mantid {
namespace Kernel {
VisibleWhenProperty::VisibleWhenProperty(std::string otherPropName,
                                         ePropertyCriterion when,
                                         std::string value)
    : EnabledWhenProperty(otherPropName, when, value) {}

bool VisibleWhenProperty::isEnabled(const IPropertyManager *) const {
  return true;
}

bool VisibleWhenProperty::isVisible(const IPropertyManager *algo) const {
  return this->fulfillsCriterion(algo);
}

IPropertySettings *VisibleWhenProperty::clone() {
  VisibleWhenProperty *out = new VisibleWhenProperty(
      m_propertyDetails->otherPropName, m_propertyDetails->criterion,
      m_propertyDetails->value);
  return out;
}

} // namespace Mantid
} // namespace Kernel
