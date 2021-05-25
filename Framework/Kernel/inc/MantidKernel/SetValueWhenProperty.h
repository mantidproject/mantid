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

  /// Return true always
  bool isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName) const override;

  /// If a new value should be set, the change is applied here
  void applyChanges(const Mantid::Kernel::IPropertyManager *algo, Kernel::Property *const pProp) override;

  /// Stub function to satisfy the interface.
  void modify_allowed_values(Property *const);

  /// Checks the algorithm and property are both valid and attempts
  /// to get the value associated with the property
  std::string getPropertyValue(const IPropertyManager *algo) const;
  /// Make a copy of the present type of IPropertySettings
  IPropertySettings *clone() const override;

private:
  /// Name of the watched property, based on which we may
  /// set the value of the current property
  std::string m_watchedPropName;
  /// Function to check if changes should be applied
  std::function<std::string(std::string, std::string)> m_changeCriterion;
};

} // namespace Kernel
} // namespace Mantid
