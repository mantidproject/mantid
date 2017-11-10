#ifndef MANTID_KERNEL_VISIBLEWHENPROPERTY_H_
#define MANTID_KERNEL_VISIBLEWHENPROPERTY_H_

#include "MantidKernel/System.h"
#include "MantidKernel/EnabledWhenProperty.h"

namespace Mantid {
namespace Kernel {

/** Same as EnabledWhenProperty, but returns the value for the
 * isVisible() property instead of the isEnabled() property.

 Copyright &copy; 2011-2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport VisibleWhenProperty : public EnabledWhenProperty {
public:
  /// Constructs a VisibleWhenProperty object which checks the property
  /// with name given and if it matches the criteria makes it visible
  VisibleWhenProperty(std::string otherPropName, ePropertyCriterion when,
                      std::string value = "");

  /// Constructs a VisibleWhenProperty object which copies two
  /// already constructed VisibleWhenProperty objects and returns the result
  /// of both of them with the specified logic operator
  VisibleWhenProperty(const VisibleWhenProperty &conditionOne,
                      const VisibleWhenProperty &conditionTwo,
                      eLogicOperator logicOperator);

  /// Constructs a VisibleWhenProperty object which takes ownership of two
  /// already constructed VisibleWhenProperty objects and returns the result
  /// of both of them with the specified logic operator
  VisibleWhenProperty(std::shared_ptr<VisibleWhenProperty> &&conditionOne,
                      std::shared_ptr<VisibleWhenProperty> &&conditionTwo,
                      eLogicOperator logicOperator);

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
  std::shared_ptr<ComparisonDetails<VisibleWhenProperty>> m_comparisonDetails =
      nullptr;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_VISIBLEWHENPROPERTY_H_ */
