#ifndef MANTID_KERNEL_PROPERTYMANAGERPROPERTY_H_
#define MANTID_KERNEL_PROPERTYMANAGERPROPERTY_H_

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/PropertyManager_fwd.h"

namespace Mantid {
namespace Kernel {

/**


  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL PropertyManagerProperty final
    : public PropertyWithValue<PropertyManager_sptr> {
public:
  // Convenience typedefs
  using BaseClass = PropertyWithValue<PropertyManager_sptr>;
  using ValueType = PropertyManager_sptr;

  PropertyManagerProperty(const std::string &name,
                          unsigned int direction = Direction::Input);
  PropertyManagerProperty(const std::string &name,
                          const ValueType &defaultValue,
                          unsigned int direction = Direction::Input);
  using BaseClass::operator=;
  PropertyManagerProperty *clone() const override {
    return new PropertyManagerProperty(*this);
  }

  std::string value() const override;
  std::string getDefault() const override;
  std::string setValue(const std::string &strValue) override;

private:
  std::string m_dataServiceKey;
  std::string m_defaultAsStr;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_PROPERTYMANAGERPROPERTY_H_ */
