// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/DateAndTimeHelpers.h"

#include <memory>
#include <stdexcept>

namespace Mantid::Kernel {

using namespace DateAndTimeHelpers;
/**
 * @return A clone of the current state of the validator
 */
IValidator_sptr DateTimeValidator::clone() const { return std::make_shared<DateTimeValidator>(*this); }

DateTimeValidator::DateTimeValidator() { m_allowedEmpty = false; }

/**
 * Sets the value of the m_allowEmpty variable
 * @param allow The new value of m_allowEmpty
 */
void DateTimeValidator::allowEmpty(const bool &allow) { m_allowedEmpty = allow; }

/**
 *  @param value A string to check for an ISO formatted timestamp
 *  @return An empty string if the value is valid or an string containing
 *          a description of the error otherwise
 */
std::string DateTimeValidator::checkValidity(const std::string &value) const {
  // simply pass off the work DateAndTime constructor
  // the DateAndTimeHelpers::stringIsISO8601 does not seem strict enough, it
  // accepts empty strings & strings of letters!
  if (m_allowedEmpty && value.empty()) {
    return "";
  } else {
    std::string error;
    try {
      Types::Core::DateAndTime timestamp(value);
      UNUSED_ARG(timestamp);
    } catch (std::invalid_argument &exc) {
      error = exc.what();
    }
    return error;
  }
}
} // namespace Mantid::Kernel
