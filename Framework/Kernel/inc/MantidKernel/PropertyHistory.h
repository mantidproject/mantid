#ifndef MANTID_KERNEL_PROPERTYHISTORY_H_
#define MANTID_KERNEL_PROPERTYHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class Property;

/** @class PropertyHistory PropertyHistory.h API/MAntidAPI/PropertyHistory.h

    This class stores information about the parameters used by an algorithm.

    @author Dickon Champion, ISIS, RAL
    @date 21/01/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL PropertyHistory {
public:
  PropertyHistory(const std::string &name, const std::string &value,
                  const std::string &type, const bool isdefault,
                  const unsigned int direction = 99);

  /// construct a property history from a property object
  PropertyHistory(Property const *const prop);
  PropertyHistory(const PropertyHistory &);
  PropertyHistory &operator=(const PropertyHistory &);
  virtual ~PropertyHistory();
  /// get name of algorithm parameter const
  const std::string &name() const { return m_name; };
  /// get value of algorithm parameter const
  const std::string &value() const { return m_value; };
  /// set value of algorithm parameter
  void setValue(const std::string &value) { m_value = value; };
  /// get type of algorithm parameter const
  const std::string &type() const { return m_type; };
  /// get isdefault flag of algorithm parameter const
  bool isDefault() const { return m_isDefault; };
  /// get direction flag of algorithm parameter const
  unsigned int direction() const { return m_direction; };
  /// print contents of object
  void printSelf(std::ostream &, const int indent = 0) const;

  /// this is required for boost.python
  bool operator==(const PropertyHistory &other) const {
    if (name() == other.name() && value() == other.value() &&
        type() == other.type() && isDefault() == other.isDefault()) {
      return true;
    }
    return false;
  }

private:
  /// The name of the parameter
  std::string m_name;
  /// The value of the parameter
  std::string m_value;
  /// The type of the parameter
  std::string m_type;
  /// flag defining if the parameter is a default or a user-defined parameter
  bool m_isDefault;
  /// direction of parameter
  unsigned int m_direction;
};

// typedefs for property history pointers
typedef boost::shared_ptr<PropertyHistory> PropertyHistory_sptr;
typedef boost::shared_ptr<const PropertyHistory> PropertyHistory_const_sptr;
typedef std::vector<PropertyHistory_sptr> PropertyHistories;

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                           const PropertyHistory &);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYHISTORY_H_*/
