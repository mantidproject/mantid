#ifndef MANTID_API_WORKSPACELISTPROPERTY_H_
#define MANTID_API_WORKSPACELISTPROPERTY_H_

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/DataItem.h"
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
  /// Type alias the value type of this property with value.
  using WorkspaceListPropertyType = std::vector<boost::shared_ptr<TYPE>>;
  using SuperClass = Kernel::PropertyWithValue<WorkspaceListPropertyType>;
  // Specify operator==
  using Kernel::PropertyWithValue<WorkspaceListPropertyType>::operator==;

  /**
  * WorkspaceListProperty
  * @param name: Name of the property
  * @param workspace: Single Workspace to hold
  * @param direction : Property directoin
  * @param optional : Optional or mandatory property
  * @param validator : Validator to use
  */
  explicit WorkspaceListProperty(
      const std::string &name, boost::shared_ptr<TYPE> workspace,
      const unsigned int direction = Mantid::Kernel::Direction::Input,
      const PropertyMode::Type optional = PropertyMode::Mandatory,
      Mantid::Kernel::IValidator_sptr validator =
          Mantid::Kernel::IValidator_sptr(new Kernel::NullValidator))
      : Mantid::Kernel::PropertyWithValue<WorkspaceListPropertyType>(
            name, WorkspaceListPropertyType(0), validator, direction),
        m_optional(optional) {
    WorkspaceListPropertyType list{workspace};
    SuperClass::operator=(list);
    auto errorMsg = isValid();
    if (!errorMsg.empty())
      throw std::invalid_argument(errorMsg);
  }

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
    SuperClass::operator=(workspaces);
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

  /**
  * Assignment overload
  * @param right : rhs workspace list property type.
  */
  WorkspaceListPropertyType &
  operator=(const WorkspaceListPropertyType &right) override {
    return SuperClass::operator=(right);
  }

  /**
  * Equivalence overload
  * @param right: rhs workspace list property type.
  */
  virtual bool operator==(const WorkspaceListProperty &right) const {
    return (m_optional == right.m_optional) && SuperClass::operator==(right);
  }

  /**
  * Clone operation.
  */
  WorkspaceListProperty<TYPE> *clone() const override {
    return new WorkspaceListProperty<TYPE>(*this);
  }

  // const WorkspaceListPropertyType  &operator() const { return
  // SuperClass::operator()(); }
  const WorkspaceListPropertyType &list() const {
    return SuperClass::operator()();
  }

  /** Get the name of the workspace
  *  @return The workspace's name
  */
  std::string value() const override { return ""; }

  /** Get the value the property was initialised with -its default value
  *  @return The default value
  */
  std::string getDefault() const override { return ""; }

  /// Is the input workspace property optional?
  bool isOptional() const { return m_optional == PropertyMode::Optional; }

  /** In addition to running the PropertyWithValue base class IValidator checks,
  * this method ensures that any WorkspaceGroups added do not exist in the ADS.
  * This check is performed because the lifetime of a workspace which both
  * exist inside the ADS and a WorkspaceGroup cannot be guaranteed. If it is
  * deleted in the ADS the shared_ptr reference is also automatically removed
  * from the WorkspaceGroup. This can lead to a dangerous situation.
  * @returns error string. Empty if no errors.
  */
  std::string isValid() const override {
    std::string error;
    auto workspaces = SuperClass::m_value;

    for (auto &wksp : workspaces) {
      auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(wksp);

      if (group) {
        auto names = group->getNames();
        if (std::any_of(names.cbegin(), names.cend(), [](const std::string &s) {
              return AnalysisDataService::Instance().doesExist(s);
            })) {
          error = "WorkspaceGroups with members in the ADS are not allowed for "
                  "WorkspaceListProperty.";
          break;
        }
      }
    }

    return error + SuperClass::isValid();
  }

  std::string
  setDataItem(const boost::shared_ptr<Kernel::DataItem> item) override {
    auto tmp = boost::dynamic_pointer_cast<TYPE>(item);
    std::string error;

    if (tmp) {
      WorkspaceListPropertyType list{tmp};
      SuperClass::m_value = list;
    } else {
      clear();
      error = "Attempted to add an invalid workspace type.";
    }

    return error + isValid();
  }

  std::string setDataItems(
      const std::vector<boost::shared_ptr<Kernel::DataItem>> &items) override {
    std::string error;

    std::vector<boost::shared_ptr<TYPE>> tmp(items.size());

    for (size_t i = 0; i < items.size(); i++)
      tmp[i] = boost::dynamic_pointer_cast<TYPE>(items[i]);

    auto valid =
        !std::any_of(tmp.cbegin(), tmp.cend(),
                     [](boost::shared_ptr<TYPE> x) { return x == nullptr; });

    if (valid) {
      SuperClass::m_value = tmp;
    } else {
      clear();
      error = "Attempted to add one of more invalid workspace types.";
    }

    return error + isValid();
  }

  /**
  * Destructor
  */
  virtual ~WorkspaceListProperty() {}

private:
  void clear() { SuperClass::operator=(WorkspaceListPropertyType(0)); }

  /// Flag indicating whether the type is optional or not.
  PropertyMode::Type m_optional;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACELISTPROPERTY_H_ */