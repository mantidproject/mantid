#include "MantidKernel/StringContainsValidator.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Kernel {

/** Constructor
 */
StringContainsValidator::StringContainsValidator() {
  m_requiredStrings = std::vector<std::string>();
}

/**
 * @return A clone of the current state of the validator
 */
IValidator_sptr StringContainsValidator::clone() const {
  return boost::make_shared<StringContainsValidator>(*this);
}

/**
 * @param strings The vector of sub strings that need to be included to pass
 * validation
 */
void StringContainsValidator::setRequiredStrings(
    const std::vector<std::string> &strings) {
  m_requiredStrings = strings;
}

/**
 *  @param value A string to check if it contains required sub strings
 *  @return An empty string if the value is valid or an string containing
 *          a description of the error otherwise
 */
std::string
StringContainsValidator::checkValidity(const std::string &value) const {
  std::string error;
  if (m_requiredStrings.empty() && !value.empty()) {
    return "";
  } else {
    if (value.empty()) {
      error += "A value must be entered for this parameter.";
    } else {
      size_t validityCount = 0;
      const size_t total = m_requiredStrings.size();
      for (size_t i = 0; i < total; i++) {
        auto position = value.find(m_requiredStrings.at(i));
        if (position != std::string::npos) {
          validityCount++;
        }
      }
      if (validityCount != total) {
        error += "Error not all the required substrings were contained within "
                 "the input '" +
                 value + "'.";
      }
    }
  }
  return error;
}

} // namespace Kernel
} // namespace Mantid
