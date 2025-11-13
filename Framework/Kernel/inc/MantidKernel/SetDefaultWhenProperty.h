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

class MANTID_KERNEL_DLL SetDefaultWhenProperty : public IPropertySettings {
public:
  /// Constructs an `SetDefaultWhenProperty` instance which sets the dynamic-default value of a
  /// property when the `changeCriterion` function is satisfied.
  SetDefaultWhenProperty(
      const std::string &watchedPropName,
      const std::function<bool(const Mantid::Kernel::IPropertyManager *, Property *, Property *)> &changeCriterion)
      : IPropertySettings(), m_watchedPropName{watchedPropName}, m_changeCriterion{changeCriterion} {}

  /// Return true when change criterion should be tested.
  bool isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName) const override;

  /// If a new value should be set, the change is applied here.
  /// Return true if current property was changed.
  bool applyChanges(const Mantid::Kernel::IPropertyManager *algo, const std::string &currentPropName) const override;

  /// Other properties that this property depends on.
  std::vector<std::string> dependsOn(const std::string &thisProp) const override;

  /// Make a copy of the present type of IPropertySettings
  IPropertySettings *clone() const override;

private:
  /// Name of the watched property.
  std::string m_watchedPropName;

  /// Criterion to determine if a new dynamic-default value should be set:
  ///   in which case this function should modify the current property's value and return True,
  ///   otherwise it should return False.
  std::function<bool(const Mantid::Kernel::IPropertyManager *, Property *, Property *)> m_changeCriterion;
};

} // namespace Kernel
} // namespace Mantid
