#include "MantidQtCustomInterfaces/ReflMeasureTransferStrategy.h"
#include <boost/regex.hpp>

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReflMeasureTransferStrategy::ReflMeasureTransferStrategy() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflMeasureTransferStrategy::~ReflMeasureTransferStrategy() {}

std::vector<std::map<std::string, std::string>>
MantidQt::CustomInterfaces::ReflMeasureTransferStrategy::transferRuns(
    const SearchResultMap &searchResults,
    Mantid::Kernel::ProgressBase &progress) {

  return std::vector<std::map<std::string, std::string>>(1); // HACK
}

ReflMeasureTransferStrategy *ReflMeasureTransferStrategy::clone() const {
  return new ReflMeasureTransferStrategy(*this);
}

bool
ReflMeasureTransferStrategy::knownFileType(const std::string &filename) const {
  boost::regex pattern("nxs$", boost::regex::icase);
  boost::smatch match; // Unused.
  return boost::regex_search(filename, match, pattern);
}

} // namespace CustomInterfaces
} // namespace Mantid
