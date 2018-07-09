#include "MantidKernel/StartsWithValidator.h"
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#endif

namespace Mantid {
namespace Kernel {
/**
 * Constructor.
 * @param values :: A vector with the allowed values.
 */
StartsWithValidator::StartsWithValidator(const std::vector<std::string> &values)
    : Kernel::StringListValidator(values) {}
/**
 * Constructor.
 * @param values :: A set with the allowed values.
 */
StartsWithValidator::StartsWithValidator(const std::set<std::string> &values)
    : Kernel::StringListValidator(values) {}

/// Clone the validator
IValidator_sptr StartsWithValidator::clone() const {
  return boost::make_shared<StartsWithValidator>(*this);
}

/** Checks if the string passed starts with one from the list
 *  @param value :: The value to test
 *  @return "" if the value is on the list, or "The value does not start with
 * any of the allowed values"
 */
std::string StartsWithValidator::checkValidity(const std::string &value) const {
  for (const auto &allowedValue : m_allowedValues) {
    if (value.substr(0, allowedValue.size()) == allowedValue) {
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
