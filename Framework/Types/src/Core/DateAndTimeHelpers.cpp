#include "MantidTypes/Core/DateAndTimeHelpers.h"

#include <boost/regex.hpp>

namespace Mantid {
namespace Types {
namespace Core {
namespace DateAndTimeHelpers {
/** Check if a string is iso8601 format.
 *
 * @param date :: string to check
 * @return true if the string conforms to ISO 860I, false otherwise.
 */
bool stringIsISO8601(const std::string &date) {
  // On Ubuntu 14.04, std::regex seems to be broken, thus boost.
  const boost::regex r(
      R"(^\d{4}-[01]\d-[0-3]\d([T\s][0-2]\d:[0-5]\d(:\d{2})?(.\d+)?(Z|[+-]\d{2}(:\d{2})?)?)?$)");
  return boost::regex_match(date, r);
}
} // namespace DateAndTimeHelpers
} // namespace Core
} // namespace Types
} // namespace Mantid
