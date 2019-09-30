// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/StartsWithValidator.h"
#ifndef Q_MOC_RUN
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
  if (std::any_of(m_allowedValues.cbegin(), m_allowedValues.cend(),
                  [&value](const auto &val) {
                    return value.substr(0, val.size()) == val;
                  })) {
    return "";
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
