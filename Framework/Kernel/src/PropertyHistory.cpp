// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Strings.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <ostream>
#include <utility>

namespace Mantid::Kernel {

/// Constructor
PropertyHistory::PropertyHistory(std::string name, std::string value, std::string type, const bool isdefault,
                                 const unsigned int direction, const bool pythonVariable)
    : m_name(std::move(name)), m_value(std::move(value)), m_type(std::move(type)), m_isDefault(isdefault),
      m_direction(direction), m_pythonVariable(pythonVariable) {}

PropertyHistory::PropertyHistory(Property const *const prop)
    : m_name(prop->name()), m_value(prop->valueAsPrettyStr(0, true)), m_type(prop->type()),
      m_isDefault(prop->isDefault()), m_direction(prop->direction()), m_pythonVariable(false) {}

/** Prints a text representation of itself
 *  @param os :: The output stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and
 * sub-objects
 *  @param maxPropertyLength :: the max length for any property value string (0
 * = full length)
 */
void PropertyHistory::printSelf(std::ostream &os, const int indent, const size_t maxPropertyLength) const {
  os << std::string(indent, ' ') << "Name: " << m_name;
  if ((maxPropertyLength > 0) && (m_value.size() > maxPropertyLength)) {
    os << ", Value: " << Strings::shorten(m_value, maxPropertyLength);
  } else {
    os << ", Value: " << m_value;
  }
  os << ", Default?: " << (m_isDefault ? "Yes" : "No");
  os << ", Direction: " << Kernel::Direction::asText(m_direction) << '\n';
}

/** Prints a text representation
 *  @param os :: The output stream to write to
 *  @param AP :: The PropertyHistory to output
 *  @returns The output stream
 */
std::ostream &operator<<(std::ostream &os, const PropertyHistory &AP) {
  AP.printSelf(os);
  return os;
}

/** Returns whether algorithm parameter was left as default EMPTY_INT,LONG,DBL
 *  @returns True if param was unset and value = EMPTY_XXX, else false
 */
bool PropertyHistory::isEmptyDefault() const {
  bool emptyDefault = false;

  // Static lists to be initialised on first call
  static std::vector<std::string> numberTypes, emptyValues;
  // Type strings corresponding to numbers
  if (numberTypes.empty()) {
    numberTypes.emplace_back(getUnmangledTypeName(typeid(int)));
    numberTypes.emplace_back(getUnmangledTypeName(typeid(int64_t)));
    numberTypes.emplace_back(getUnmangledTypeName(typeid(double)));
  }
  // Empty values
  if (emptyValues.empty()) {
    emptyValues.emplace_back(std::to_string(EMPTY_INT()));
    emptyValues.emplace_back(boost::lexical_cast<std::string>(EMPTY_DBL()));
    emptyValues.emplace_back(std::to_string(EMPTY_LONG()));
  }

  // If default, input, number type and matches empty value then return true
  if (m_isDefault && m_direction != Direction::Output) {
    if (std::find(numberTypes.begin(), numberTypes.end(), m_type) != numberTypes.end()) {
      if (std::find(emptyValues.begin(), emptyValues.end(), m_value) != emptyValues.end()) {
        emptyDefault = true;
      }
    }
  }
  return emptyDefault;
}

} // namespace Mantid::Kernel
