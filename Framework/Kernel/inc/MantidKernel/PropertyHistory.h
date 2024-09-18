// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <memory>

#include <iosfwd>
#include <string>
#include <vector>

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
*/
class MANTID_KERNEL_DLL PropertyHistory {
public:
  PropertyHistory(std::string name, std::string value, std::string type, const bool isdefault,
                  const unsigned int direction = 99, const bool pythonVariable = false);

  /// construct a property history from a property object
  PropertyHistory(Property const *const prop);
  /// destructor
  virtual ~PropertyHistory() = default;
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
  void printSelf(std::ostream &, const int indent = 0, const size_t maxPropertyLength = 0) const;
  /// get whether algorithm parameter was left as default EMPTY_INT,LONG,DBL
  /// const
  bool isEmptyDefault() const;
  bool pythonVariable() const { return m_pythonVariable; };

  /// this is required for boost.python
  bool operator==(const PropertyHistory &other) const {
    return name() == other.name() && value() == other.value() && type() == other.type() &&
           isDefault() == other.isDefault();
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
  /// Whether the property should be treated as a python variable instead of string when building a script from history
  bool m_pythonVariable;
};

// typedefs for property history pointers
using PropertyHistory_sptr = std::shared_ptr<PropertyHistory>;
using PropertyHistory_const_sptr = std::shared_ptr<const PropertyHistory>;
using PropertyHistories = std::vector<PropertyHistory_sptr>;

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const PropertyHistory &);

} // namespace Kernel
} // namespace Mantid
