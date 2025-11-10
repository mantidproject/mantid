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

namespace {

Mantid::Kernel::Logger g_log("SetValueWhenProperty");

} // namespace

namespace Mantid::Kernel {

/**
 * Return true if the `changedPropName` matches the `watchedPropName`.
 *
 * @param algo :: Pointer to the algorithm containing the property
 * @param changedPropName :: Name of the property that has just been changed
 * @return  :: True if the Property we are watching is the property that just changed, otherwise False
 */
bool SetValueWhenProperty::isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName) const {
  const auto *watchedProp = algo->getPointerToProperty(m_watchedPropName);
  return watchedProp->name() == changedPropName;
}

bool SetValueWhenProperty::applyChanges(const IPropertyManager *algo, const std::string &currentPropName) {
  try {
    auto *currentProperty = algo->getPointerToProperty(currentPropName);
    const auto *watchedProperty = algo->getPointerToProperty(m_watchedPropName);
    const std::string currentValue = currentProperty->value();
    const std::string watchedValue = watchedProperty->value();

    std::string newValue = m_changeCriterion(currentValue, watchedValue);
    if (newValue != currentValue) {
      currentProperty->setValue(newValue);
      return true;
    }
    return false;
  } catch (const Exception::NotFoundError &) {
    g_log.warning() << "`SetValueWhenProperty::applyChanges`: one of the properies '" << currentPropName << "' or '"
                    << m_watchedPropName << "\n"
                    << "   is not present on the algorithm.\n";
  } catch (const std::runtime_error &e) {
    // Note: the exception might be a `PythonException`, but `MantidPythonInterface/core`
    //   is not accessible at this level.
    g_log.warning() << "`SetValueWhenProperty::applyChanges`: exception thrown within provided callback.\n"
                    << "If the callback was a Python `Callable`, the stack trace follows:\n"
                    << "-------------------------------------------------------------------------\n"
                    << e.what() << "\n-------------------------------------------------------------------------\n";
  }
  return false;
}

/// Other properties that this property depends on.
std::vector<std::string> SetValueWhenProperty::dependsOn(const std::string &thisProp) const {
  if (m_watchedPropName == thisProp)
    throw std::runtime_error("SetValueWhenProperty: circular dependency detected");
  return std::vector<std::string>{m_watchedPropName};
}

IPropertySettings *SetValueWhenProperty::clone() const { return new SetValueWhenProperty(*this); }

} // namespace Mantid::Kernel
