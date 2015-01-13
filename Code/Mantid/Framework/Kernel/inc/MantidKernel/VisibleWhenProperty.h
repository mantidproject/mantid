#ifndef MANTID_KERNEL_VISIBLEWHENPROPERTY_H_
#define MANTID_KERNEL_VISIBLEWHENPROPERTY_H_

#include "MantidKernel/System.h"
#include "MantidKernel/EnabledWhenProperty.h"

namespace Mantid {
namespace Kernel {

/** Same as EnabledWhenProperty, but returns the value for the
 * isVisible() property intead of the isEnabled() property.

  @author Janik Zikovsky
  @date 2011-08-26

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
class DLLExport VisibleWhenProperty : public EnabledWhenProperty {
public:
  //--------------------------------------------------------------------------------------------
  /** Constructor
   * @param otherPropName :: Name of the OTHER property that we will check.
   * @param when :: Criterion to evaluate
   * @param value :: For the IS_EQUAL_TO or IS_NOT_EQUAL_TO condition, the value
   * (as string) to check for
   */
  VisibleWhenProperty(std::string otherPropName, ePropertyCriterion when,
                      std::string value = "")
      : EnabledWhenProperty(otherPropName, when, value) {}

  /// Destructor
  virtual ~VisibleWhenProperty() {}

  //--------------------------------------------------------------------------------------------
  /// Return true always
  virtual bool isEnabled(const IPropertyManager *) const { return true; }

  //--------------------------------------------------------------------------------------------
  /// Return true/false based on whether the other property satisfies the
  /// criterion
  virtual bool isVisible(const IPropertyManager *algo) const {
    return this->fulfillsCriterion(algo);
  }

  //--------------------------------------------------------------------------------------------
  /// Make a copy of the present type of validator
  virtual IPropertySettings *clone() {
    VisibleWhenProperty *out = new VisibleWhenProperty(
        this->m_otherPropName, this->m_when, this->m_value);
    return out;
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_VISIBLEWHENPROPERTY_H_ */
