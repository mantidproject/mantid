//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/StartsWithValidator.h"

namespace Mantid {
namespace Kernel {

/** Checks if the string passed starts with one from the list
  *  @param value :: The value to test
  *  @return "" if the value is on the list, or "The value does not start with
 * any of the allowed values"
  */
std::string StartsWithValidator::checkValidity(const std::string &value) const {
  for (auto it = m_allowedValues.begin(); it != m_allowedValues.end(); ++it) {
    if (value.substr(0, it->size()) == *it) {
      return "";
    }
  }
  if (isEmpty(value))
    return "Select a value";
  std::ostringstream os;
  os << "The value \"" << value
     << "\" does not start with any of the allowed values";
  return os.str();
}

} // namespace Kernel
} // namespace Mantid
