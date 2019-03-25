// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflMeasureTransferStrategy.h"
#include "MantidKernel/ICatalogInfo.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/UserCatalogInfo.h"
#include "ReflMeasurementItemSource.h"
#include "ReflTableSchema.h"
#include <boost/regex.hpp>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

using namespace Mantid::Kernel;

namespace MantidQt {
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReflMeasureTransferStrategy::ReflMeasureTransferStrategy(
    std::unique_ptr<ICatalogInfo> catInfo,
    std::unique_ptr<ReflMeasurementItemSource> measurementItemSource)
    : m_catInfo(std::move(catInfo)),
      m_measurementItemSource(std::move(measurementItemSource)) {}

ReflMeasureTransferStrategy::ReflMeasureTransferStrategy(
    const ReflMeasureTransferStrategy &other)
    : m_catInfo(other.m_catInfo->clone()),
      m_measurementItemSource(other.m_measurementItemSource->clone())

{}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflMeasureTransferStrategy::~ReflMeasureTransferStrategy() {}

TransferResults
MantidQt::CustomInterfaces::ReflMeasureTransferStrategy::transferRuns(
    SearchResultMap &searchResults, Mantid::Kernel::ProgressBase &progress,
    const TransferMatch matchType) {
  UNUSED_ARG(matchType);

  using VecSameMeasurement = std::vector<MeasurementItem>;
  using MapGroupedMeasurement =
      std::map<MeasurementItem::IDType, VecSameMeasurement>;

  // table-like output for successful runs
  std::vector<std::map<std::string, std::string>> runs;
  // table-like output for unsuccessful runs containing
  // row number and reason why unsuccessful.
  // This will be used mainly for highlighting unsuccessful runs
  // in a tooltip.
  std::vector<std::map<std::string, std::string>> errors;

  // Create TransferResults as a holder for both successful/unsuccessful
  // runs.
  TransferResults results(runs, errors);
  MapGroupedMeasurement mapOfMeasurements;
  for (auto & searchResult : searchResults) {
    const auto location = searchResult.second.location;
    const auto fuzzyName = searchResult.first;

    const auto definedPath = m_catInfo->transformArchivePath(location);

    // This is where we read the meta data.
    MeasurementItem metaData =
        m_measurementItemSource->obtain(definedPath, fuzzyName);

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
      searchResult.second.issues = metaData.whyUnuseable();
      // run was unsuccessful so add it to 'errors'
      results.addErrorRow(metaData.run(), metaData.whyUnuseable());
    }

    // Obtaining metadata could take time.
    progress.report();
  }

  int nextGroupId = 0;

  for (auto & mapOfMeasurement : mapOfMeasurements) {

    std::string groupName;

    // Map keyed by subId to index of exisiting subid written.
    std::map<std::string, size_t> subIdMap;
    for (size_t i = 0; i < mapOfMeasurement.second.size(); ++i) {
      const MeasurementItem &measurementItem = mapOfMeasurement.second[i];

      if (i == 0) {
        std::string title = measurementItem.title();
        groupName = std::to_string(nextGroupId) + " - " +
                    title.substr(0, title.find(":th"));
      }

      if (subIdMap.find(measurementItem.subId()) != subIdMap.end()) {
        // We already have that subid.
        const size_t rowIndex = subIdMap[measurementItem.subId()];
        std::string currentRuns =
            results.m_transferRuns[rowIndex][ReflTableSchema::RUNS];
        results.m_transferRuns[rowIndex][ReflTableSchema::RUNS] =
            currentRuns + "+" + measurementItem.run();

      } else {
        // set up our successful run row
        std::map<std::string, std::string> row;
        row[ReflTableSchema::RUNS] = measurementItem.run();
        row[ReflTableSchema::ANGLE] = measurementItem.angleStr();
        row[ReflTableSchema::GROUP] = groupName;
        // run was successful so add it to 'runs'
        results.addTransferRow(row);
        // get successful transfers to get size for subIdMap
        auto transRuns = results.getTransferRuns();
        subIdMap.insert(
            std::make_pair(measurementItem.subId(),
                           transRuns.size() - 1 /*Record actual row index*/));
      }
    }
    ++nextGroupId;
  }
  // return the TransferResults holder
  return results;
}

std::unique_ptr<ReflMeasureTransferStrategy>
ReflMeasureTransferStrategy::clone() const {
  return std::unique_ptr<ReflMeasureTransferStrategy>(doClone());
}

ReflMeasureTransferStrategy *ReflMeasureTransferStrategy::doClone() const {
  return new ReflMeasureTransferStrategy(*this);
}

bool ReflMeasureTransferStrategy::knownFileType(
    const std::string &filename) const {

  // TODO. I think this file type matching should be deferred to the
  // ReflMeasurementSource

  boost::regex pattern("nxs$", boost::regex::icase);
  boost::smatch match; // Unused.
  return boost::regex_search(filename, match, pattern);
}

} // namespace CustomInterfaces
} // namespace MantidQt
