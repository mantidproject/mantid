// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SetValueWhenProperty.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/Property.h"

#include <boost/lexical_cast.hpp>
#include <cassert>
#include <exception>
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid::Kernel {

/**
 * Checks the validity of the algorithm and the property:
 * returns the value of the watched property.
 *
 * @param algo :: The pointer to the algorithm to process
 * @return :: The value of the watched property
 * @throw :: Throws if anything is wrong with the property or algorithm
 */
std::string SetValueWhenProperty::getPropertyValue(const IPropertyManager *algo) const {
  // Find the property
  if (algo == nullptr)
    throw std::runtime_error("Algorithm properties passed to SetValueWhenProperty was null");

  Property const *prop = nullptr;
  try {
    prop = algo->getPointerToProperty(m_watchedPropName);
  } catch (Exception::NotFoundError &) {
    prop = nullptr; // Ensure we still have null pointer
  }
  if (!prop)
    throw std::runtime_error("Property " + m_watchedPropName + " was not found in SetValueWhenProperty");
  return prop->value();
}

/**
 * Return true if the `changedPropName` matches the `watchedPropName`.
 *
 * @param algo :: Pointer to the algorithm containing the property
 * @param changedPropName :: Name of the property that has just been changed
 * @return  :: True if the Property we are watching is the property that just changed, otherwise False
 */
bool SetValueWhenProperty::isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName) const {
  auto const *watchedProp = algo->getPointerToProperty(m_watchedPropName);
  bool hasWatchedPropChanged = false;
  if (watchedProp->name() == changedPropName) {
    hasWatchedPropChanged = true;
  }
  return hasWatchedPropChanged;
}

void SetValueWhenProperty::applyChanges(const IPropertyManager *algo, const std::string &currentPropName) {
  const auto currentProperty = algo->getPointerToProperty(currentPropName);
  const std::string watchedPropertyValue = getPropertyValue(algo);

  std::string newValue = m_changeCriterion(currentProperty->value(), watchedPropertyValue);
  currentProperty->setValue(newValue);
}

/// Other properties that this property depends on.
std::vector<std::string> SetValueWhenProperty::dependsOn(const std::string &thisProp) const {
  if (m_watchedPropName == thisProp)
    throw std::runtime_error("SetValueWhenProperty: circular dependency detected");
  return std::vector<std::string>{m_watchedPropName};
}

#if 0 // *** DEBUG *** => this seems deprecated?
/// Does nothing in this case and put here to satisfy the interface.
void SetValueWhenProperty::modify_allowed_values(Property *const /*unused*/) {}
#endif

IPropertySettings *SetValueWhenProperty::clone() const { return new SetValueWhenProperty(*this); }

} // namespace Mantid::Kernel
