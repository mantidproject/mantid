#ifndef MANTID_KERNEL_ENABLEDWHENPROPERTY_H_
#define MANTID_KERNEL_ENABLEDWHENPROPERTY_H_

#include "MantidKernel/System.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include <memory>

namespace Mantid {
namespace Kernel {

/** Enum for use in EnabledWhenProperty */
enum ePropertyCriterion {
  IS_DEFAULT,
  IS_NOT_DEFAULT,
  IS_EQUAL_TO,
  IS_NOT_EQUAL_TO,
  IS_MORE_OR_EQ
};

/** Enum for use when combining two EnabledWhenPropertyItems */
enum eLogicOperator { AND, OR, XOR };

/** IPropertySettings for a property that sets it to enabled (in the GUI)
   when the value of another property is:
    - its default (or not)
    - equal to a string (or not)

    Usage:

      - In an algorithm's init() method, after a call to create a property:

      declareProperty("PropA", 123);

      - Add a call like this:

      setPropertySettings("PropA",
  make_unique<EnabledWhenProperty>("OtherProperty",
  IS_EQUAL_TO, "2000");

      - This will make the property "PropA" show as enabled when
  "OtherProperty"'s value is equal to "2000". Similarly, you can use:

      setPropertySettings("PropA",
  make_unique<VisibleWhenProperty>("OtherProperty",
  IS_NOT_DEFAULT);

      - This will make the property "PropA" show as visible when "OtherProperty"
  is NOT the default value for it.


  @author Janik Zikovsky
  @date 2011-08-25

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport EnabledWhenProperty : public IPropertySettings {
public:
  //--------------------------------------------------------------------------------------------
  /** Constructor
   * @param otherPropName :: Name of the OTHER property that we will check.
   * @param when :: Criterion to evaluate
   * @param value :: For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value
   * (as string) to check for
   */
  EnabledWhenProperty(const std::string &otherPropName,
                      const ePropertyCriterion when,
                      const std::string &value = "");

  /** Multiple conditions constructor - takes two unique pointers to
    * EnabledWhenProperty objects and returns the product of them
        * with the specified logic operator.
        * Note: With Unique pointers you will need to use std::move
        * to transfer ownership of those objects to this one.
    *
    * @param conditionOne :: First EnabledWhenProperty object to use
    * @param conditionTwo :: Second EnabledWhenProperty object to use
    * @param localOperator :: The logic operator to apply across both
    *conditions
    *
    */
  EnabledWhenProperty(std::unique_ptr<EnabledWhenProperty> &&conditionOne,
                      std::unique_ptr<EnabledWhenProperty> &&conditionTwo,
                      eLogicOperator logicalOperator);

  //--------------------------------------------------------------------------------------------
  /** Does the validator fulfill the criterion based on the
   * other property values?
   * @return true if fulfilled or if any problem was found (missing property,
   * e.g.).
   */
  virtual bool checkCriterion(const IPropertyManager *algo) const;

  /**
   * Checks if a the user specified combination of criterion
   * returns a true or false value
   *
   * @return true if user specified combination was true.
   * @throw If any problems was found
   */
  virtual bool checkComparison(const IPropertyManager *algo) const;

  //--------------------------------------------------------------------------------------------
  /// Return true/false based on whether the other property satisfies the
  /// criterion
  bool isEnabled(const IPropertyManager *algo) const override;

  //--------------------------------------------------------------------------------------------
  /// Return true always
  bool isVisible(const IPropertyManager *) const override;
  /// does nothing in this case and put here to satisfy the interface.
  void modify_allowed_values(Property *const);
  //--------------------------------------------------------------------------------------------
  /// Make a copy of the present type of validator
  IPropertySettings *clone() override;

protected:
  std::string getPropertyValue(const IPropertyManager *algo) const;

  struct PropertyDetails {
    PropertyDetails(const std::string &otherPropName,
                    const ePropertyCriterion criterion,
                    const std::string &value)
        : otherPropName(otherPropName), criterion(criterion), value(value) {}
    /// Name of the OTHER property that we will check.
    const std::string otherPropName;
    /// Criterion to evaluate
    const ePropertyCriterion criterion;
    /// For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value (as string)
    /// to
    /// check for
    const std::string value;
  };

  struct ComparisonDetails {
    ComparisonDetails(std::unique_ptr<EnabledWhenProperty> conditionOne,
                      std::unique_ptr<EnabledWhenProperty> conditionTwo,
                      eLogicOperator logicOperator)
        : conditionOne(std::move(conditionOne)),
          conditionTwo(std::move(conditionTwo)), logicOperator(logicOperator) {}

    std::unique_ptr<EnabledWhenProperty> conditionOne;
    std::unique_ptr<EnabledWhenProperty> conditionTwo;
    const eLogicOperator logicOperator;
  };

  // Holds the various details used within the comparison
  std::unique_ptr<PropertyDetails> m_propertyDetails = nullptr;
  std::unique_ptr<ComparisonDetails> m_comparisonDetails = nullptr;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_ENABLEDWHENPROPERTY_H_ */
