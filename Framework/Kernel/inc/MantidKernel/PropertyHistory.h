// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROPERTYHISTORY_H_
#define MANTID_KERNEL_PROPERTYHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <boost/shared_ptr.hpp>

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
  PropertyHistory(const std::string &name, const std::string &value,
                  const std::string &type, const bool isdefault,
                  const unsigned int direction = 99);

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
  void printSelf(std::ostream &, const int indent = 0,
                 const size_t maxPropertyLength = 0) const;
  /// get whether algorithm parameter was left as default EMPTY_INT,LONG,DBL
  /// const
  bool isEmptyDefault() const;

  /// this is required for boost.python
  bool operator==(const PropertyHistory &other) const {
    return name() == other.name() && value() == other.value() &&
           type() == other.type() && isDefault() == other.isDefault();
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
using PropertyHistory_sptr = boost::shared_ptr<PropertyHistory>;
using PropertyHistory_const_sptr = boost::shared_ptr<const PropertyHistory>;
using PropertyHistories = std::vector<PropertyHistory_sptr>;

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                           const PropertyHistory &);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYHISTORY_H_*/
