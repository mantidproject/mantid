// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTypes/Core/DateAndTimeHelpers.h"

#include <boost/regex.hpp>

namespace Mantid {
namespace Types {
namespace Core {
namespace DateAndTimeHelpers {
/** Check if a string is in ISO8601 format.
 *
 * @param date :: string to check
 * @return true if the string conforms to ISO 8601, false otherwise.
 */
bool stringIsISO8601(const std::string &date) {
  // Expecting most of Mantid's time stamp strings to be in the
  // extended format --- check it first.
  static const boost::regex extendedFormat(
      R"(^\d{4}-[01]\d-[0-3]\d([T\s][0-2]\d:[0-5]\d(:\d{2})?(.\d+)?(Z|[+-]\d{2}(:?\d{2})?)?)?$)");
  if (!boost::regex_match(date, extendedFormat)) {
    static const boost::regex basicFormat(
        R"(^\d{4}[01]\d[0-3]\d([T\s][0-2]\d[0-5]\d(\d{2})?(.\d+)?(Z|[+-]\d{2}(:?\d{2})?)?)?$)");
    return boost::regex_match(date, basicFormat);
  }
  return true;
}

/** Check if a string is in POSIX format.
 *
 * @param date :: string to check
 * @return true if the string conforms to POSIX, false otherwise.
 */
bool stringIsPosix(const std::string &date) {
  // Formatting taken from boost::to_simple_string.
  static const boost::regex format(R"(^\d{4}-[A-Z][a-z]{2}-[0-3]\d\s[0-2]\d:[0-5]\d:\d{2}(.\d+)?$)");
  return boost::regex_match(date, format);
}
} // namespace DateAndTimeHelpers
} // namespace Core
} // namespace Types
} // namespace Mantid
