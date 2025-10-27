// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/SetDefaultWhenProperty.h"

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

Mantid::Kernel::Logger g_log("SetDefaultWhenProperty");

} // namespace

namespace Mantid::Kernel {

/**
 * Return true if the `changedPropName` matches the `watchedPropName`.
 *
 * @param algo :: Pointer to the algorithm containing the property
 * @param changedPropName :: Name of the property that has just been changed
 * @return  :: True if the changed property is the property that we are watching
 */
bool SetDefaultWhenProperty::isConditionChanged(const IPropertyManager *algo,
                                                const std::string &changedPropName) const {
  UNUSED_ARG(algo);
  return changedPropName == m_watchedPropName;
}

bool SetDefaultWhenProperty::applyChanges(const IPropertyManager *algo, const std::string &currentPropName) {
  try {
    auto currentProperty = algo->getPointerToProperty(currentPropName);
    auto watchedProperty = algo->getPointerToProperty(m_watchedPropName);

    // For this `IPropertySettings`: only apply changes if the property still has its default value,
    //   or its value has been set programmatically.
    if (!(currentProperty->isDefault() || currentProperty->isDynamicDefault()))
      return false;

    if (m_changeCriterion(algo, currentProperty, watchedProperty)) {
      currentProperty->setIsDynamicDefault(true);
      return true;
    }
  } catch (const Exception::NotFoundError &) {
    g_log.warning() << "`SetDefaultWhenProperty::applyChanges`: one of the properies '" << currentPropName << "' or '"
                    << m_watchedPropName << "\n"
                    << "   is not present on the algorithm.\n";
  } catch (const std::runtime_error &e) {
    // Note: the exception might be a `PythonException`, but `MantidPythonInterface/core`
    //   is not accessible at this level.
    g_log.warning() << "`SetDefaultWhenProperty::applyChanges`: exception thrown within provided callback.\n"
                    << "If the callback was a Python `Callable`, the stack trace follows:\n"
                    << "-------------------------------------------------------------------------\n"
                    << e.what() << "\n-------------------------------------------------------------------------\n";
  }
  return false;
}

/// Other properties that this property depends on.
std::vector<std::string> SetDefaultWhenProperty::dependsOn(const std::string &thisProp) const {
  if (m_watchedPropName == thisProp)
    throw std::runtime_error("SetDefaultWhenProperty: circular dependency detected");
  return std::vector<std::string>{m_watchedPropName};
}

IPropertySettings *SetDefaultWhenProperty::clone() const { return new SetDefaultWhenProperty(*this); }

} // namespace Mantid::Kernel
