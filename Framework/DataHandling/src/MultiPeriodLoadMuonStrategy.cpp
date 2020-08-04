// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/MultiPeriodLoadMuonStrategy.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexusV2Helper.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace NeXus;
using namespace DataObjects;

namespace {
std::string GOODFRAMESPROP{"goodfrm"};
constexpr bool MULTIPERIODSLOADED = true;
} // namespace

// Constructor
MultiPeriodLoadMuonStrategy::MultiPeriodLoadMuonStrategy(
    Kernel::Logger &g_log, const std::string filename, NXEntry &entry,
    API::WorkspaceGroup_sptr workspace)
    : LoadMuonStrategy(g_log, filename), m_entry(entry),
      m_workspaceGroup(std::move(workspace)),
      m_detectors(getLoadedDetectors()) {}

/**
 * Loads Muon specific logs into each of the workspaces in the workspace group.
 * These are logs which are not loaded by LoadISISNexus
 */
void MultiPeriodLoadMuonStrategy::loadMuonLogData() {

  auto sampleInformation =
      LoadMuonNexusV2Helper::loadSampleInformationFromNexus(m_entry);
  std::string mainFieldDirection =
      LoadMuonNexusV2Helper::loadMainFieldDirectionFromNexus(m_entry);
  double firstGoodData =
      LoadMuonNexusV2Helper::loadFirstGoodDataFromNexus(m_entry);

  for (int i = 0; i < m_workspaceGroup->getNumberOfEntries(); ++i) {
    auto workspace =
        std::dynamic_pointer_cast<Workspace2D>(m_workspaceGroup->getItem(i));
    auto &run = workspace->mutableRun();
    run.addProperty("main_field_direction", mainFieldDirection);
    run.addProperty("FirstGoodData", firstGoodData);
    run.addProperty("sample_temp", sampleInformation.temperature);
    run.addProperty("sample_magn_field", sampleInformation.magneticField);
  }
}
/**
 * Loads the good frames data into each of the stored workspace objects
 */
void MultiPeriodLoadMuonStrategy::loadGoodFrames() {
  NXInt goodframes = LoadMuonNexusV2Helper::loadGoodFramesDataFromNexus(
      m_entry, MULTIPERIODSLOADED);
  for (int i = 0; i < m_workspaceGroup->getNumberOfEntries(); ++i) {
    auto workspace =
        std::dynamic_pointer_cast<Workspace2D>(m_workspaceGroup->getItem(i));
    auto &run = workspace->mutableRun();
    run.removeProperty(GOODFRAMESPROP);
    run.addProperty(GOODFRAMESPROP, goodframes[i]);
  }
}

/**
 * Loads detector grouping.
 * If no entry in NeXus file for grouping, load it from the IDF.
 * stored
 * @returns :: TableWorkspace group containing each of the grouping tables
 */
Workspace_sptr MultiPeriodLoadMuonStrategy::loadDetectorGrouping() const {
  // Each period could in theory have its own grouping, which is reflected in
  // the nexus file which has (periods*numDetectors) entries in the Nexus
  // grouping entry.
  WorkspaceGroup_sptr tableGroup = std::make_shared<WorkspaceGroup>();
  for (int i = 0; i < m_workspaceGroup->getNumberOfEntries(); ++i) {
    int periodNumber = i + 1;
    auto grouping = LoadMuonNexusV2Helper::loadDetectorGroupingFromNexus(
        m_entry, m_detectors, MULTIPERIODSLOADED, periodNumber);
    TableWorkspace_sptr table =
        createDetectorGroupingTable(m_detectors, grouping);
    // if any of the tables are empty we'll load grouping from the IDF
    if (table->rowCount() == 0) {
      m_logger.notice("Loading grouping information from IDF");
      return LoadMuonNexusV2Helper::loadDefaultDetectorGrouping(
          std::dynamic_pointer_cast<Workspace2D>(m_workspaceGroup->getItem(i)));
    } else {
      tableGroup->addWorkspace(table);
    }
  }
  return tableGroup;
}
/**
 * Performs time-zero correction on the loaded workspace.
 */
void MultiPeriodLoadMuonStrategy::applyTimeZeroCorrection() {
  double timeZero = LoadMuonNexusV2Helper::loadTimeZeroFromNexusFile(m_entry);
  for (int i = 0; i < m_workspaceGroup->getNumberOfEntries(); ++i) {
    auto workspace =
        std::dynamic_pointer_cast<Workspace2D>(m_workspaceGroup->getItem(i));
    auto numHistograms = workspace->getNumberHistograms();
    for (size_t i = 0; i < numHistograms; ++i) {
      auto &timeAxis = workspace->mutableX(i);
      timeAxis = timeAxis - timeZero;
    }
  }
}
// Load dead time table
API::Workspace_sptr MultiPeriodLoadMuonStrategy::loadDeadTimeTable() const {
  WorkspaceGroup_sptr tableGroup = std::make_shared<WorkspaceGroup>();
  for (int i = 0; i < m_workspaceGroup->getNumberOfEntries(); ++i) {
    int periodNumber = i + 1;
    auto deadTimes = LoadMuonNexusV2Helper::loadDeadTimesFromNexus(
        m_entry, m_detectors, MULTIPERIODSLOADED, periodNumber);
    tableGroup->addWorkspace(createDeadTimeTable(m_detectors, deadTimes));
  }
  return tableGroup;
};

/**
 * Finds the detectors which are loaded in the stored workspace group
 */
std::vector<detid_t> MultiPeriodLoadMuonStrategy::getLoadedDetectors() {
  // Assume each spectrum maps to the same detector in each period.
  auto workspace =
      std::dynamic_pointer_cast<Workspace2D>(m_workspaceGroup->getItem(0));
  return LoadMuonNexusV2Helper::getLoadedDetectorsFromWorkspace(workspace);
}

} // namespace DataHandling
} // namespace Mantid
