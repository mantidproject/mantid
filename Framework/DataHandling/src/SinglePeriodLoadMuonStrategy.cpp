// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SinglePeriodLoadMuonStrategy.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexusV2NexusHelper.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid::DataHandling {

using namespace API;
using namespace NeXus;
using std::size_t;
using namespace DataObjects;

// Constructor
SinglePeriodLoadMuonStrategy::SinglePeriodLoadMuonStrategy(Kernel::Logger &g_log, const std::string &filename,
                                                           LoadMuonNexusV2NexusHelper &nexusLoader,
                                                           Workspace2D &workspace, int entryNumber,
                                                           bool isFileMultiPeriod)
    : LoadMuonStrategy(g_log, filename, nexusLoader), m_workspace(workspace), m_entryNumber(entryNumber),
      m_isFileMultiPeriod(isFileMultiPeriod), m_detectors(getLoadedDetectors()) {}

/**
 * Loads Muon specific logs into the stored workspace
 * These are logs which are not loaded by LoadISISNexus
 */
void SinglePeriodLoadMuonStrategy::loadMuonLogData() {

  auto &run = m_workspace.mutableRun();
  auto sampleInformation = m_nexusLoader.loadSampleInformationFromNexus();
  std::string mainFieldDirection = m_nexusLoader.loadMainFieldDirectionFromNexus();
  double firstGoodData = m_nexusLoader.loadFirstGoodDataFromNexus();

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
  auto &run = m_workspace.mutableRun();
  run.removeProperty(goodframeProp);
  NXInt goodframes = m_nexusLoader.loadGoodFramesDataFromNexus(m_isFileMultiPeriod);

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
  auto const grouping = m_nexusLoader.loadDetectorGroupingFromNexus(m_detectors, m_isFileMultiPeriod, m_entryNumber);
  auto const table = createDetectorGroupingTable(m_detectors, grouping);
  if (table && (*table)->rowCount() != 0) {
    return *table;
  }
  return loadDefaultDetectorGrouping(m_workspace);
}
/**
 * Loads deadtime table from nexus file
 * @returns :: Dead time table
 */
Workspace_sptr SinglePeriodLoadMuonStrategy::loadDeadTimeTable() const {
  auto deadTimes = m_nexusLoader.loadDeadTimesFromNexus(m_detectors, m_isFileMultiPeriod, m_entryNumber);
  return createDeadTimeTable(m_detectors, deadTimes);
}
/**
 * Gets time zero table from loaded time zeros
 * @returns :: Time zero table
 */
Workspace_sptr SinglePeriodLoadMuonStrategy::getTimeZeroTable() {
  const auto numSpec = m_workspace.getNumberHistograms();
  auto timeZeros = m_nexusLoader.loadTimeZeroListFromNexusFile(numSpec);
  return createTimeZeroTable(numSpec, timeZeros);
}
/**
 * Performs time-zero correction on the loaded workspace.
 */
void SinglePeriodLoadMuonStrategy::applyTimeZeroCorrection() {
  double timeZero = m_nexusLoader.loadTimeZeroFromNexusFile();
  auto numHistograms = m_workspace.getNumberHistograms();
  for (size_t i = 0; i < numHistograms; ++i) {
    auto &timeAxis = m_workspace.mutableX(i);
    timeAxis = timeAxis - timeZero;
  }
}
/**
 * Finds the detectors which are loaded in the stored workspace
 */
std::vector<detid_t> SinglePeriodLoadMuonStrategy::getLoadedDetectors() {
  return getLoadedDetectorsFromWorkspace(m_workspace);
}
} // namespace Mantid::DataHandling
