// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IPropertySettings.h"
#include <functional>
#include <string>

namespace Mantid {
namespace Kernel {

// Forward declarations
class IPropertyManager;
class Property;

class MANTID_KERNEL_DLL SetValueWhenProperty : public IPropertySettings {
public:
  /// Constructs an SetValueWhenProperty object which checks the
  /// watched property with name given and uses the given
  /// function to check if changes should be applied
  SetValueWhenProperty(const std::string &watchedPropName,
                       const std::function<std::string(std::string, std::string)> &changeCriterion)
      : IPropertySettings(), m_watchedPropName{watchedPropName}, m_changeCriterion{changeCriterion} {}

  /// Return true when the property changed is the property that the current property's value depends on.
  bool isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName) const override;

  /// If a new value should be set, the change is applied here.
  /// Return true if the current property was actually changed.
  bool applyChanges(const Mantid::Kernel::IPropertyManager *algo, const std::string &currentPropName) const override;

  /// Other properties that this property depends on.
  std::vector<std::string> dependsOn(const std::string &thisProp) const override;

  /// Make a copy of the present type of IPropertySettings
  IPropertySettings *clone() const override;

private:
  /// Name of the watched property, which is the property that the current property's value depends on.
  std::string m_watchedPropName;

  /// Callback to check and actually apply any required changes:
  /// this function returns a new string value for the current property,
  /// which in some cases might be the same as the current value.
  std::function<std::string(std::string, std::string)> m_changeCriterion;
};

} // namespace Kernel
} // namespace Mantid
