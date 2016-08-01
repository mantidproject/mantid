#ifndef MANTID_API_WORKSPACELISTPROPERTY_H_
#define MANTID_API_WORKSPACELISTPROPERTY_H_

#include "MantidAPI/WorkspaceProperty.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <vector>

namespace Mantid {
namespace API {

/** WorkspaceListProperty : Property with value that constrains the contents to
  be a list of workspaces of a single type.

  @author Owen Arnold, ISIS, RAL
  @date 03/10/2013

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

template <typename TYPE = MatrixWorkspace>
class DLLExport WorkspaceListProperty
    : public Mantid::Kernel::PropertyWithValue<
          std::vector<boost::shared_ptr<TYPE>>> {
public:
  /// Typedef the value type of this property with value.
  typedef std::vector<boost::shared_ptr<TYPE>> WorkspaceListPropertyType;
  typedef Kernel::PropertyWithValue<WorkspaceListPropertyType> SuperClass;

  /**
  * WorkspaceListProperty
  * @param name: Name of the property
  * @param workspaces: Workspaces to hold
  * @param direction : Property directoin
  * @param optional : Optional or mandatory property
  * @param validator : Validator to use
  */
  explicit WorkspaceListProperty(
      const std::string &name, const WorkspaceListPropertyType workspaces,
      const unsigned int direction = Mantid::Kernel::Direction::Input,
      const PropertyMode::Type optional = PropertyMode::Mandatory,
      Mantid::Kernel::IValidator_sptr validator =
          Mantid::Kernel::IValidator_sptr(new Kernel::NullValidator))
      : Mantid::Kernel::PropertyWithValue<WorkspaceListPropertyType>(
            name, WorkspaceListPropertyType(0), validator, direction),
        m_optional(optional) {
    Kernel::PropertyWithValue<WorkspaceListPropertyType>::m_value = workspaces;
    auto errorMsg = isValid();
    if (!errorMsg.empty())
      throw std::invalid_argument(errorMsg);
  }

  /// Copy constructor, the default name stored in the new object is the same as
  /// the default name from the original object
  WorkspaceListProperty(const WorkspaceListProperty &right)
      : SuperClass(right), m_optional(right.m_optional) {}

  /**
  * Assignment overload
  * @param right : rhs workspace list property.
  */
  WorkspaceListProperty &operator=(const WorkspaceListProperty &right) {
    if (&right != this) {
      SuperClass::operator=(right);
      m_optional = right.m_optional;
    }
    return *this;
  }

  std::string value() const override { return ""; }

  /**
  * Assignment overload
  * @param right : rhs workspace list property type.
  */
  WorkspaceListPropertyType &operator=(const WorkspaceListPropertyType &right) {
    return SuperClass::operator=(right);
  }

  /**
  * Clone operation.
  */
  WorkspaceListProperty<TYPE> *clone() const {
    return new WorkspaceListProperty<TYPE>(*this);
  }

  /** Checks whether the entered workspaces are valid.
  *  To be valid, in addition to satisfying the conditions of any validators.
  * Input ones must point to
  *  workspaces of the correct type.
  *  @returns A user level description of the problem or "" if it is valid.
  */
  std::string isValid() const {
    std::string error;
    // Run the validator on each workspace held. This must be done after the
    // point that we have established that the workspaces exist.
    for (int i = 0; i < SuperClass::m_value.size(); ++i) {
      error = SuperClass::m_validator->isValid(SuperClass::m_value[i]);
      if (!error.empty()) {
        break;
      }
    }
    return error;
  }

  const WorkspaceListPropertyType &value() { return SuperClass::m_value; }

  /// Is the input workspace property optional?
  virtual bool isOptional() const {
    return m_optional == PropertyMode::Optional;
  }

  /**
  * Destructor
  */
  virtual ~WorkspaceListProperty() {}

private:
  void clear() { SuperClass::m_value = WorkspaceListPropertyType(0); }

  /// Flag indicating whether the type is optional or not.
  PropertyMode::Type m_optional;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACELISTPROPERTY_H_ */