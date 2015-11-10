//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/Property.h"

#include <sstream>

namespace Mantid {
namespace Kernel {

/// Constructor
PropertyHistory::PropertyHistory(const std::string &name,
                                 const std::string &value,
                                 const std::string &type, const bool isdefault,
                                 const unsigned int direction)
    : m_name(name), m_value(value), m_type(type), m_isDefault(isdefault),
      m_direction(direction) {}

PropertyHistory::PropertyHistory(Property const *const prop)
    : // PropertyHistory::PropertyHistory(prop->name(), prop->value(),
      // prop->type(), prop->isDefault(), prop->direction())
      m_name(prop->name()),
      m_value(prop->value()), m_type(prop->type()),
      m_isDefault(prop->isDefault()), m_direction(prop->direction()) {}

/// Destructor
PropertyHistory::~PropertyHistory() {}

/**
 * Standard Copy Constructor
 * @param A :: PropertyHistory Item to copy
 */
PropertyHistory::PropertyHistory(const PropertyHistory &A)
    : m_name(A.m_name), m_value(A.m_value), m_type(A.m_type),
      m_isDefault(A.m_isDefault), m_direction(A.m_direction) {}

/**
 * Standard Assignment operator
 * @param A :: PropertyHistory Item to assign to 'this'
 * @return pointer to this
 */
PropertyHistory &PropertyHistory::operator=(const PropertyHistory &A) {
  if (this != &A) {
    m_name = A.m_name;
    m_value = A.m_value;
    m_type = A.m_type;
    m_isDefault = A.m_isDefault;
    m_direction = A.m_direction;
  }
  return *this;
}

/** Prints a text representation of itself
 *  @param os :: The ouput stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and
 * sub-objects
 */
void PropertyHistory::printSelf(std::ostream &os, const int indent) const {
  os << std::string(indent, ' ') << "Name: " << m_name;
  os << ", Value: " << m_value;
  os << ", Default?: " << (m_isDefault ? "Yes" : "No");
  os << ", Direction: " << Kernel::Direction::asText(m_direction) << std::endl;
}

/** Prints a text representation
 *  @param os :: The ouput stream to write to
 *  @param AP :: The PropertyHistory to output
 *  @returns The ouput stream
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
    numberTypes.push_back(getUnmangledTypeName(typeid(int)));
    numberTypes.push_back(getUnmangledTypeName(typeid(int64_t)));
    numberTypes.push_back(getUnmangledTypeName(typeid(double)));
  }
  // Empty values
  if (emptyValues.empty()) {
    emptyValues.push_back(boost::lexical_cast<std::string>(EMPTY_INT()));
    emptyValues.push_back(boost::lexical_cast<std::string>(EMPTY_DBL()));
    emptyValues.push_back(boost::lexical_cast<std::string>(EMPTY_LONG()));
  }

  // If default, input, number type and matches empty value then return true
  if (m_isDefault && m_direction != Direction::Output) {
    if (std::find(numberTypes.begin(), numberTypes.end(), m_type) !=
        numberTypes.end()) {
      if (std::find(emptyValues.begin(), emptyValues.end(), m_value) !=
          emptyValues.end()) {
        emptyDefault = true;
      }
    }
  }
  return emptyDefault;
}

} // namespace Kernel
} // namespace Mantid
