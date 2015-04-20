#include "MantidKernel/EnabledWhenProperty.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {
EnabledWhenProperty::EnabledWhenProperty(std::string otherPropName,
                                         ePropertyCriterion when,
                                         std::string value)
    : IPropertySettings(), m_otherPropName(otherPropName), m_when(when),
      m_value(value) {}

EnabledWhenProperty::~EnabledWhenProperty() {}

bool
EnabledWhenProperty::fulfillsCriterion(const IPropertyManager *algo) const {
  // Find the property
  if (algo == NULL)
    return true;
  Property *prop = NULL;
  try {
    prop = algo->getPointerToProperty(m_otherPropName);
  } catch (Exception::NotFoundError &) {
    return true; // Property not found. Ignore
  }
  if (!prop)
    return true;

  // Value of the other property
  std::string propValue = prop->value();

  // OK, we have the property. Check the condition
  switch (m_when) {
  case IS_DEFAULT:
    return prop->isDefault();
  case IS_NOT_DEFAULT:
    return !prop->isDefault();
  case IS_EQUAL_TO:
    return (propValue == m_value);
  case IS_NOT_EQUAL_TO:
    return (propValue != m_value);
  case IS_MORE_OR_EQ: {
    int check = boost::lexical_cast<int>(m_value);
    int iPropV = boost::lexical_cast<int>(propValue);
    return (iPropV >= check);
  }
  default:
    // Unknown criterion
    return true;
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
  EnabledWhenProperty *out =
      new EnabledWhenProperty(m_otherPropName, m_when, m_value);
  return out;
}

} // namespace Mantid
} // namespace Kernel
