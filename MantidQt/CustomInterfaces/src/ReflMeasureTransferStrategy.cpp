#include "MantidQtCustomInterfaces/ReflMeasureTransferStrategy.h"
#include "MantidQtCustomInterfaces/ReflMeasurementSource.h"
#include "MantidQtCustomInterfaces/ReflTableSchema.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/UserCatalogInfo.h"
#include <boost/regex.hpp>
#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <limits>
#include <set>
#include <sstream>

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
    SearchResultMap &searchResults, Mantid::Kernel::ProgressBase &progress) {

  typedef std::vector<Measurement> VecSameMeasurement;
  typedef std::map<Measurement::IDType, VecSameMeasurement>
      MapGroupedMeasurement;

  MapGroupedMeasurement mapOfMeasurements;
  for (auto it = searchResults.begin(); it != searchResults.end(); ++it) {
    const auto location = it->second.location;
    const auto fuzzyName = it->first;

    const auto definedPath = m_catInfo->transformArchivePath(location);

    // This is where we read the meta data.
    Measurement metaData = m_measurementSource->obtain(definedPath, fuzzyName);

    // If the measurement information is not consistent, or could not be
    // obtained. skip this measurement.
    if (metaData.isUseable()) {
      if (mapOfMeasurements.find(metaData.id()) == mapOfMeasurements.end()) {
        // Start a new group
        mapOfMeasurements.insert(
            std::make_pair(metaData.id(), VecSameMeasurement(1, metaData)));
      } else {
        // Add to existing group
        mapOfMeasurements[metaData.id()].push_back(metaData);
      }
    } else {
      it->second.issues = metaData.whyUnuseable();
    }

    // Obtaining metadata could take time.
    progress.report();
  }

  // Now flatten everything out into a table-like output
  std::vector<std::map<std::string, std::string>> output;
  int nextGroupId = 0;

  for (auto group = mapOfMeasurements.begin(); group != mapOfMeasurements.end();
       ++group) {

    // Map keyed by subId to index of exisiting subid written.
    std::map<std::string, size_t> subIdMap;
    for (size_t i = 0; i < group->second.size(); ++i) {
      const Measurement &measurement = group->second[i];
      if (subIdMap.find(measurement.subId()) != subIdMap.end()) {
        // We already have that subid.
        const size_t rowIndex = subIdMap[measurement.subId()];
        std::string currentRuns = output[rowIndex][ReflTableSchema::RUNS];
        output[rowIndex][ReflTableSchema::RUNS] =
            currentRuns + "+" + measurement.run();
      } else {
        std::map<std::string, std::string> row;
        row[ReflTableSchema::RUNS] = measurement.run();
        row[ReflTableSchema::ANGLE] = measurement.angleStr();
        std::stringstream buffer;
        buffer << nextGroupId;
        row[ReflTableSchema::GROUP] = buffer.str();
        subIdMap.insert(std::make_pair(measurement.subId(), i));
        output.push_back(row);
      }
    }
    ++nextGroupId;
  }

  return output;
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
