// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SinglePeriodLoadMuonStrategy.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexusV2Helper.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;
using namespace DataObjects;

// Constructor
SinglePeriodLoadMuonStrategy::SinglePeriodLoadMuonStrategy(
    Kernel::Logger &g_log, const std::string filename, NXEntry &entry,
    Workspace2D_sptr workspace, int entryNumber, bool isFileMultiPeriod)
    : LoadMuonStrategy(g_log, filename), m_entry(entry),
      m_workspace(std::move(workspace)), m_entryNumber(entryNumber),
      m_isFileMultiPeriod(isFileMultiPeriod),
      m_detectors(getLoadedDetectors()) {}

/**
 * Loads Muon specific logs into the stored workspace
 * These are logs which are not loaded by LoadISISNexus
 */
void SinglePeriodLoadMuonStrategy::loadMuonLogData() {

  auto &run = m_workspace->mutableRun();
  auto sampleInformation =
      LoadMuonNexusV2Helper::loadSampleInformationFromNexus(m_entry);
  std::string mainFieldDirection =
      LoadMuonNexusV2Helper::loadMainFieldDirectionFromNexus(m_entry);
  double firstGoodData =
      LoadMuonNexusV2Helper::loadFirstGoodDataFromNexus(m_entry);

  run.addProperty("sample_temp", sampleInformation.temperature);
  run.addProperty("sample_magn_field", sampleInformation.magneticField);
  run.addProperty("main_field_direction", mainFieldDirection);
  run.addProperty("FirstGoodData", firstGoodData);
}

/**
 * Loads the good frames data into the stored workspace object
 */
void SinglePeriodLoadMuonStrategy::loadGoodFrames() {
  // Overwrite existing log entry
  std::string goodframeProp{"goodfrm"};
  auto &run = m_workspace->mutableRun();
  run.removeProperty(goodframeProp);
  NXInt goodframes = LoadMuonNexusV2Helper::loadGoodFramesDataFromNexus(
      m_entry, m_isFileMultiPeriod);

  if (m_isFileMultiPeriod) {
    run.addProperty(goodframeProp, goodframes[m_entryNumber - 1]);
  } else {
    run.addProperty(goodframeProp, goodframes[0]);
  }
}

/**
 * Loads detector grouping.
 * If no entry in NeXus file for grouping, load it from the IDF.
 * stored
 * @returns :: Grouping table
 */
Workspace_sptr SinglePeriodLoadMuonStrategy::loadDetectorGrouping() const {

  auto grouping = LoadMuonNexusV2Helper::loadDetectorGroupingFromNexus(
      m_entry, m_detectors, m_isFileMultiPeriod, m_entryNumber);
  TableWorkspace_sptr table =
      createDetectorGroupingTable(m_detectors, grouping);

  if (table->rowCount() != 0) {
    return table;
  } else {
    m_logger.notice("Loading grouping information from IDF");
    return LoadMuonNexusV2Helper::loadDefaultDetectorGrouping(m_workspace);
  }
}
/**
 * Loads deadtime table from nexus file
 * @returns :: Dead time table
 */
Workspace_sptr SinglePeriodLoadMuonStrategy::loadDeadTimeTable() const {
  auto deadTimes = LoadMuonNexusV2Helper::loadDeadTimesFromNexus(
      m_entry, m_detectors, m_isFileMultiPeriod, m_entryNumber);
  return createDeadTimeTable(m_detectors, deadTimes);
}
/**
 * Performs time-zero correction on the loaded workspace.
 */
void SinglePeriodLoadMuonStrategy::applyTimeZeroCorrection() {
  double timeZero = LoadMuonNexusV2Helper::loadTimeZeroFromNexusFile(m_entry);
  auto numHistograms = m_workspace->getNumberHistograms();
  for (size_t i = 0; i < numHistograms; ++i) {
    auto &timeAxis = m_workspace->mutableX(i);
    timeAxis = timeAxis - timeZero;
  }
}
/**
 * Finds the detectors which are loaded in the stored workspace
 */
std::vector<detid_t> SinglePeriodLoadMuonStrategy::getLoadedDetectors() {
  return LoadMuonNexusV2Helper::getLoadedDetectorsFromWorkspace(m_workspace);
}
} // namespace DataHandling
} // namespace Mantid
