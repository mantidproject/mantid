#include "MantidGeometry/Instrument/ComponentVisitorHelper.h"
#include <boost/regex.hpp>
#include <string>

namespace Mantid {
namespace Geometry {
namespace ComponentVisitorHelper {

/**
 * Does name imply pack of tubes
 * @param nameHint
 * @return True if matches naming
 */
bool matchesPackOfTubes(const std::string &nameHint) {
  using boost::regex;
  static regex expression("^[a-z]*pack$", regex::icase);
  return boost::regex_match(nameHint, expression);
}

/**
 * Does name imply PSD tube
 * @param nameHint
 * @return True if matches naming
 */
bool matchesPSDTube(const std::string &nameHint) {
  using boost::regex;
  static regex expression("^tube[0-9]*$", regex::icase);
  return boost::regex_match(nameHint, expression);
}

} // namespace ComponentVisitorHelper
} // namespace Geometry
} // namespace Mantid
