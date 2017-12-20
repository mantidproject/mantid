#include "MantidGeometry/Instrument/ComponentVisitorHelper.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include "MantidGeometry/ICompAssembly.h"
#include <boost/regex.hpp>

namespace Mantid {
namespace Geometry {
namespace ComponentVisitorHelper {

namespace {

/**
 * Does name imply pack of tubes
 * @param nameHint
 * @return True if matches naming
 */
bool matchesPackOfTubes(const std::string &nameHint) {
  using boost::regex;
  return boost::regex_match(nameHint, regex("^[a-z]*pack$", regex::icase));
}

/**
 * Does name imply PSD tube
 * @param nameHint
 * @return True if matches naming
 */
bool matchesPSDTube(const std::string &nameHint) {
  using boost::regex;
  return boost::regex_match(nameHint, regex("^tube[0-9]*$", regex::icase));
}
}

/**
 * Determines and calls the appropriate vistor method relating to an
 *IComponentAssembly
 *
 * The IComponentAssembly hierachy is not rich enough to describe the full range
 *of common components.
 * It is desirable to know some of these as there are speedups that can be
 *performed in downstream processing if you know say that the component you have
 *is say a psd tube.
 *
 * This method is written to be conservative in it's approach to matching.
 *Failure to match based on the hint will lead to correct, albeit slower,
 *behaviour.
 *
 * @param visitor : The mutable vistor on which the appropriate register method
 *will be called
 * @param visitee : The visitee that will be interrogated
 * @param nameHint : Naming hint that can be used to determine what type of
 *Component we have
 */
size_t visitAssembly(ComponentVisitor &visitor, const ICompAssembly &visitee,
                     const std::string &nameHint) {
  using boost::regex;
  if (matchesPackOfTubes(nameHint)) {
    return visitor.registerBankOfTubes(visitee);
  } else if (matchesPSDTube(nameHint)) {
    return visitor.registerTube(visitee);
  }
  // Generic Assembly registration call.
  return visitor.registerComponentAssembly(visitee);
}
} // namespace ComponentVisitorHelper
} // namespace Geometry
} // namespace Mantid
