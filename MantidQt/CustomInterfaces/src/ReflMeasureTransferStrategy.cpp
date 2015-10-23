#include "MantidQtCustomInterfaces/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/ReflMeasurementSource.h"

#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/UserCatalogInfo.h"
#include <boost/regex.hpp>
#include <memory>

using namespace Mantid::Kernel;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReflMeasureTransferStrategy::ReflMeasureTransferStrategy(
    std::unique_ptr<ICatalogInfo> catInfo,
    std::unique_ptr<ReflMeasurementSource> measurementSource)
    : m_catInfo(std::move(catInfo)),
      m_measurementSource(std::move(measurementSource)) {}

ReflMeasureTransferStrategy::ReflMeasureTransferStrategy(
    const ReflMeasureTransferStrategy &other)
    : m_catInfo(other.m_catInfo->clone()),
      m_measurementSource(other.m_measurementSource->clone())

{}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflMeasureTransferStrategy::~ReflMeasureTransferStrategy() {}

std::vector<std::map<std::string, std::string>>
MantidQt::CustomInterfaces::ReflMeasureTransferStrategy::transferRuns(
    const SearchResultMap &searchResults,
    Mantid::Kernel::ProgressBase &progress) {

  for (auto it = searchResults.begin(); it != searchResults.end(); ++it) {
    const auto location = it->second.location;
    const auto fuzzyName = it->first;

    const auto definedPath = m_catInfo->transformArchivePath(location);

    Measurement metaData = m_measurementSource->obtain(definedPath, fuzzyName);
    /*
    const Poco::File filePath(loadPath);
    if (filePath.exists() && filePath.isFile()) {


    } else {
        // Load from this path
    }
    */

    progress.report();
  }
  return std::vector<std::map<std::string, std::string>>(1); // HACK
}

ReflMeasureTransferStrategy *ReflMeasureTransferStrategy::clone() const {
  return new ReflMeasureTransferStrategy(*this);
}

bool
ReflMeasureTransferStrategy::knownFileType(const std::string &filename) const {

  // TODO. I think this file type matching should be deferred to the
  // ReflMeasurementSource

  boost::regex pattern("nxs$", boost::regex::icase);
  boost::smatch match; // Unused.
  return boost::regex_search(filename, match, pattern);
}

} // namespace CustomInterfaces
} // namespace Mantid
