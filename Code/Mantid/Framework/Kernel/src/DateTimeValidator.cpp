//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/DateAndTime.h"

namespace Mantid {
namespace Kernel {

/**
 * @return A clone of the current state of the validator
 */
IValidator_sptr DateTimeValidator::clone() const {
  return boost::make_shared<DateTimeValidator>(*this);
}

/**
 *  @param value A string to check for an ISO formatted timestamp
 *  @return An empty string if the value is valid or an string containing
 *          a description of the error otherwise
 */
std::string DateTimeValidator::checkValidity(const std::string &value) const {
  // simply pass off the work DateAndTime constructor
  // the DateAndTime::stringIsISO8601 does not seem strict enough, it accepts
  // empty strings & strings of letters!
  std::string error("");
  try {
    DateAndTime timestamp(value);
    UNUSED_ARG(timestamp);
  } catch (std::invalid_argument &exc) {
    error = exc.what();
  }
  return error;
}
}
}
