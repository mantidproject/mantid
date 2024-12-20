// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include <stdexcept>

namespace Mantid {
namespace Kernel {

/** Same as EnabledWhenProperty, but returns the value for the
 * isVisible() property instead of the isEnabled() property.
 */
class MANTID_KERNEL_DLL VisibleWhenProperty : public EnabledWhenProperty {
public:
  /// Constructs a VisibleWhenProperty object which checks the property
  /// with name given and if it matches the criteria makes it visible
  VisibleWhenProperty(const std::string &otherPropName, ePropertyCriterion when, const std::string &value = "");

  /// Constructs a VisibleWhenProperty object which copies two
  /// already constructed VisibleWhenProperty objects and returns the result
  /// of both of them with the specified logic operator
  VisibleWhenProperty(const VisibleWhenProperty &conditionOne, const VisibleWhenProperty &conditionTwo,
                      eLogicOperator logicOperator);

  /// Constructs a VisibleWhenProperty object which takes ownership of two
  /// already constructed VisibleWhenProperty objects and returns the result
  /// of both of them with the specified logic operator
  VisibleWhenProperty(std::shared_ptr<VisibleWhenProperty> &&conditionOne,
                      std::shared_ptr<VisibleWhenProperty> &&conditionTwo, eLogicOperator logicOperator);

  /// Checks two VisisbleWhenProperty objects to determine the
  /// result of both of them
  virtual bool checkComparison(const IPropertyManager *algo) const override;

  /// Return true always as we only consider visible
  bool isEnabled(const IPropertyManager *algo) const override;

  /// Return true/false based on whether the other property satisfies the
  /// criterion
  bool isVisible(const IPropertyManager *algo) const override;

  /// Make a copy of the present type of validator
  IPropertySettings *clone() const override;

private:
  /// Hold a copy of any existing VisibleWhenPropertyObjects
  std::shared_ptr<ComparisonDetails<VisibleWhenProperty>> m_comparisonDetails = nullptr;
};

} // namespace Kernel
} // namespace Mantid
