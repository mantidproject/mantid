// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IPropertySettings.h"
#include <memory>
#include <string>

namespace Mantid {
namespace Kernel {

/** IPropertySettings for a property that sets it to enabled (in the GUI)
when the value of another property is:
- its default (or not)
- equal to a string (or not)

Usage:

- In an algorithm's init() method, after a call to create a property:

declareProperty("PropA", 123);

- Add a call like this:

setPropertySettings("PropA",
std::make_unique<EnabledWhenProperty>("OtherProperty",
IS_EQUAL_TO, "2000");

- This will make the property "PropA" show as enabled when
"OtherProperty"'s value is equal to "2000". Similarly, you can use:

setPropertySettings("PropA",
std::make_unique<VisibleWhenProperty>("OtherProperty",
IS_NOT_DEFAULT);

- To combine them you create objects as detailed above and combine them
in the constructor
setPropertySettings("PropA",
std::make_unique<VisibleWhenProperty>(conditionOne, conditionTwo, AND));

- This will make the property "PropA" show as visible when "OtherProperty"
is NOT the default value for it.
*/

// Forward decelerations
class IPropertyManager;
class Property;

// Forward deceleration of structs defined at end of header

/** Enum for use in EnabledWhenProperty */
enum ePropertyCriterion { IS_DEFAULT, IS_NOT_DEFAULT, IS_EQUAL_TO, IS_NOT_EQUAL_TO, IS_MORE_OR_EQ };

/** Enum for use when combining two EnabledWhenPropertyItems */
enum eLogicOperator { AND, OR, XOR };

class MANTID_KERNEL_DLL EnabledWhenProperty : public IPropertySettings {
public:
  /// Constructs a EnabledWhenProperty object which checks the property
  /// with name given and if it matches the criteria enables it
  EnabledWhenProperty(const std::string &otherPropName, const ePropertyCriterion when, const std::string &value = "");

  /// Constructs a EnabledWhenProperty object which copies two
  /// already constructed EnabledWhenProperty objects and returns the result
  /// of both of them with the specified logic operator
  EnabledWhenProperty(const EnabledWhenProperty &conditionOne, const EnabledWhenProperty &conditionTwo,
                      eLogicOperator logicOperator);

  /// Constructs a EnabledWhenProperty object which takes ownership of two
  /// already constructed EnabledWhenProperty objects and returns the result
  /// of both of them with the specified logic operator
  EnabledWhenProperty(std::shared_ptr<EnabledWhenProperty> &&conditionOne,
                      std::shared_ptr<EnabledWhenProperty> &&conditionTwo, eLogicOperator logicOperator);

  /// Copy constructor
  EnabledWhenProperty(const EnabledWhenProperty &other);

  /// Checks two EnabledWhenProperty objects match the logic operator
  /// specified and returns the result of both of them
  virtual bool checkComparison(const IPropertyManager *algo) const;

  /// Checks that the specified property matches the criteria given
  virtual bool checkCriterion(const IPropertyManager *algo) const;

  /// Return true/false based on whether the other property satisfies the
  /// criterion
  bool isEnabled(const IPropertyManager *algo) const override;

  /// Return true always
  bool isVisible(const IPropertyManager *algo) const override;

  /// Stub function to satisfy the interface.
  void modify_allowed_values(Property *const);

  /// Make a copy of the present type of validator
  IPropertySettings *clone() const override;

protected:
  /// Struct which holds associated property details for comparison
  struct PropertyDetails {
    /// Name of the OTHER property that we will check.
    const std::string otherPropName;
    /// Criterion to evaluate
    const ePropertyCriterion criterion;
    /// For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition,
    /// the value (as string) to check for
    const std::string value;
  };

  /// Struct which holds details for comparison between
  /// two EnabledWhenPropertyObjects
  template <typename WhenPropType> struct ComparisonDetails {
    std::shared_ptr<WhenPropType> conditionOne;
    std::shared_ptr<WhenPropType> conditionTwo;
    const eLogicOperator logicOperator;
  };

  /// Protected Constructor for derived classes to skip setting up
  /// the comparator in the base class as they will handle it
  EnabledWhenProperty() = default;

  /// Checks the algorithm and property are both valid and attempts
  /// to get the value associated with the property
  std::string getPropertyValue(const IPropertyManager *algo) const;

  /// Holds the various details used within the comparison
  std::shared_ptr<PropertyDetails> m_propertyDetails = nullptr;

private:
  /// Holds an object containing details of multiple comparisons
  std::shared_ptr<ComparisonDetails<EnabledWhenProperty>> m_comparisonDetails = nullptr;
};

} // namespace Kernel
} // namespace Mantid
