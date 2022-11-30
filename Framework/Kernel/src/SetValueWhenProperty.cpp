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
 * Checks the algorithm given is in a valid state and the property
 * exists then proceeds to try to get the value associated
 *
 * @param algo :: The pointer to the algorithm to process
 * @return :: The value contained by said property
 * @throw :: Throws if anything is wrong with the property or algorithm
 */
std::string SetValueWhenProperty::getPropertyValue(const IPropertyManager *algo) const {
  // Find the property
  if (algo == nullptr)
    throw std::runtime_error("Algorithm properties passed to SetValueWhenProperty was null");

  Property *prop = nullptr;
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
 * Always returns true as SetValueWhenProperty always wants to check
 * if the property it depends on has changed
 *
 * @param algo :: Pointer to the algorithm containing the property
 * @param changedPropName :: Name of the property that has just been changed by the user
 * @return  :: True if the Property we are watching is the property that just changed, otherwise False
 */
bool SetValueWhenProperty::isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName) const {
  auto watchedProp = algo->getPointerToProperty(m_watchedPropName);
  bool hasWatchedPropChanged = false;
  if (watchedProp->name() == changedPropName) {
    hasWatchedPropChanged = true;
  }

  return hasWatchedPropChanged;
}

void SetValueWhenProperty::applyChanges(const IPropertyManager *algo, Kernel::Property *const currentProperty) {
  const std::string watchedPropertyValue = getPropertyValue(algo);

  std::string newValue = m_changeCriterion(currentProperty->value(), watchedPropertyValue);
  currentProperty->setValue(newValue);
}

/// Does nothing in this case and put here to satisfy the interface.
void SetValueWhenProperty::modify_allowed_values(Property *const /*unused*/) {}

IPropertySettings *SetValueWhenProperty::clone() const { return new SetValueWhenProperty(*this); }

} // namespace Mantid::Kernel
