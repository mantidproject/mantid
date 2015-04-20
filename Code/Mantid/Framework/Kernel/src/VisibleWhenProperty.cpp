#include "MantidKernel/VisibleWhenProperty.h"

namespace Mantid {
namespace Kernel {
  VisibleWhenProperty::VisibleWhenProperty(std::string otherPropName, ePropertyCriterion when,
                      std::string value)
      : EnabledWhenProperty(otherPropName, when, value) {}

      VisibleWhenProperty::~VisibleWhenProperty() {}

       bool VisibleWhenProperty::isEnabled(const IPropertyManager *) const { return true; }

       bool VisibleWhenProperty::isVisible(const IPropertyManager *algo) const {
        return this->fulfillsCriterion(algo);
      }

       IPropertySettings *VisibleWhenProperty::clone() {
        VisibleWhenProperty *out = new VisibleWhenProperty(
            this->m_otherPropName, this->m_when, this->m_value);
        return out;
      }

} // namespace Mantid
} // namespace Kernel
