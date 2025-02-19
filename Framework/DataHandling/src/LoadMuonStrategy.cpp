// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadMuonStrategy.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses.h"
#include <vector>

namespace Mantid::DataHandling {

/**
 * Creates a timezero table for the loaded detectors
 * @param numSpec :: Number of spectra (number of rows in table)
 * @param timeZeros :: Vector containing time zero values for each spectra
 * @return TableWorkspace of time zeros
 */
DataObjects::TableWorkspace_sptr createTimeZeroTable(const size_t numSpec, const std::vector<double> &timeZeros) {
  Mantid::DataObjects::TableWorkspace_sptr timeZeroTable =
      std::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(
          Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace"));
  timeZeroTable->addColumn("double", "time zero");

  for (size_t specNum = 0; specNum < numSpec; ++specNum) {
    Mantid::API::TableRow row = timeZeroTable->appendRow();
    row << timeZeros[specNum];
  }

  return timeZeroTable;
}

// Constructor
LoadMuonStrategy::LoadMuonStrategy(Kernel::Logger &g_log, std::string filename, LoadMuonNexusV2NexusHelper &nexusLoader)
    : m_logger(g_log), m_filename(std::move(filename)), m_nexusLoader(nexusLoader) {}

/**
 * Loads default detector grouping, if this isn't present
 * return dummy grouping
 * @returns :: Grouping table
 */
API::Workspace_sptr
LoadMuonStrategy::loadDefaultDetectorGrouping(const DataObjects::Workspace2D &localWorkspace) const {
  m_logger.information("Loading grouping information from IDF");

  auto instrument = localWorkspace.getInstrument();
  auto &run = localWorkspace.run();
  std::string mainFieldDirection = run.getLogData("main_field_direction")->value();
  API::GroupingLoader groupLoader(instrument, mainFieldDirection);
  try {
    const auto idfGrouping = groupLoader.getGroupingFromIDF();
    return idfGrouping->toTable();
  } catch (const std::runtime_error &) {
    auto dummyGrouping = std::make_shared<API::Grouping>();
    if (instrument->getNumberDetectors() != 0) {
      dummyGrouping = groupLoader.getDummyGrouping();
    } else {
      // Make sure it uses the right number of detectors
      std::string numDetectors = "1-" + std::to_string(localWorkspace.getNumberHistograms());
      dummyGrouping->groups.emplace_back(std::move(numDetectors));
      dummyGrouping->groupNames.emplace_back("all");
    }
    return dummyGrouping->toTable();
  }
}
/**
 * Determines the detectors loaded in the input workspace.
 * @returns :: Vector containing loaded detectors
 */
std::vector<detid_t>
LoadMuonStrategy::getLoadedDetectorsFromWorkspace(const DataObjects::Workspace2D &localWorkspace) const {
  size_t numberOfSpectra = localWorkspace.getNumberHistograms();
  std::vector<detid_t> loadedDetectors;
  loadedDetectors.reserve(numberOfSpectra);
  for (size_t spectraIndex = 0; spectraIndex < numberOfSpectra; spectraIndex++) {
    const auto detIdSet = localWorkspace.getSpectrum(spectraIndex).getDetectorIDs();
    // each spectrum should only point to one detector in the Muon file
    loadedDetectors.emplace_back(*detIdSet.begin());
  }
  return loadedDetectors;
}

/**
 * Creates Detector Grouping Table .
 * @param detectorsLoaded :: Vector containing the list of detectorsLoaded
 * @param grouping :: Vector containing corresponding grouping
 * @return Detector Grouping Table create using the data
 */
std::optional<DataObjects::TableWorkspace_sptr>
LoadMuonStrategy::createDetectorGroupingTable(const std::vector<detid_t> &detectorsLoaded,
                                              const std::optional<std::vector<detid_t>> &grouping) const {
  if (!grouping) {
    m_logger.information("No grouping information is provided in the Nexus file");
    return std::nullopt;
  }
  auto groupingIDs = *grouping;
  if (detectorsLoaded.size() != groupingIDs.size()) {
    m_logger.information() << "The number of groupings in the provided Nexus file (" << groupingIDs.size()
                           << ") does not match the number of loaded detectors (" << detectorsLoaded.size() << ").";
    return std::nullopt;
  }

  std::map<detid_t, std::vector<detid_t>> groupingMap;
  for (size_t i = 0; i < detectorsLoaded.size(); ++i) {
    // Add detector ID to the list of group detectors. Detector ID is always
    groupingMap[groupingIDs[i]].emplace_back(detectorsLoaded[i]);
  }

  auto detectorGroupingTable = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(
      API::WorkspaceFactory::Instance().createTable("TableWorkspace"));
  detectorGroupingTable->addColumn("vector_int", "Detectors");
  for (const auto &group : groupingMap) {
    if (group.first != 0) { // Skip 0 group
      API::TableRow newRow = detectorGroupingTable->appendRow();
      newRow << group.second;
    }
  }
  return detectorGroupingTable;
}
/**
 * Creates the deadtime table for the loaded detectors .
 * @param detectorsLoaded :: Vector containing the list of detectorsLoaded
 * @param deadTimes :: Vector containing corresponding deadtime
 * @return Deadtime table created using the input data
 */
DataObjects::TableWorkspace_sptr LoadMuonStrategy::createDeadTimeTable(const std::vector<detid_t> &detectorsLoaded,
                                                                       const std::vector<double> &deadTimes) const {
  auto deadTimesTable = std::dynamic_pointer_cast<DataObjects::TableWorkspace>(
      API::WorkspaceFactory::Instance().createTable("TableWorkspace"));

  deadTimesTable->addColumn("int", "spectrum");
  deadTimesTable->addColumn("double", "dead-time");

  for (size_t i = 0; i < detectorsLoaded.size(); i++) {
    API::TableRow row = deadTimesTable->appendRow();
    row << detectorsLoaded[i] << deadTimes[i];
  }

  return deadTimesTable;
}

} // namespace Mantid::DataHandling
