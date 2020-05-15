
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SinglePeriodLoadMuonStrategy.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidDataHandling/LoadMuonLog.h"
#include "MantidDataHandling/LoadMuonNexus3Helper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;
using namespace DataObjects;

// Constructor
SinglePeriodLoadMuonStrategy::SinglePeriodLoadMuonStrategy(
    Kernel::Logger &g_log, const std::string &filename, NXEntry entry,
    Workspace2D_sptr workspace, int entryNumber, bool isFileMultiPeriod)
    : LoadMuonStrategy(g_log, filename), m_entry(entry), m_workspace(workspace),
      m_detectors(getLoadedDetectors()), m_entryNumber(entryNumber),
      m_isFileMultiPeriod(isFileMultiPeriod) {}

// Loads MuonLogData for a single period file
void SinglePeriodLoadMuonStrategy::loadMuonLogData() {

  auto loadMuonLogs = AlgorithmFactory::Instance().create("LoadMuonLog", 1);
  loadMuonLogs->initialize();
  // Pass through the same input filename
  loadMuonLogs->setAlwaysStoreInADS(false);
  loadMuonLogs->setProperty("Filename", m_filename);
  loadMuonLogs->setProperty<MatrixWorkspace_sptr>("Workspace", m_workspace);
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    m_logger.notice("Loading Muon Log data");
    loadMuonLogs->executeAsChildAlg();
  } catch (std::runtime_error &) {
    m_logger.error("Unable to successfully run LoadMuonLog Child Algorithm");
  }
  std::string mainFieldDirection =
      LoadMuonNexus3Helper::loadMainFieldDirectionFromNexus(m_entry);
  // set output property and add to workspace logs
  auto &run = m_workspace->mutableRun();
  run.addProperty("main_field_direction", mainFieldDirection);
}

/**
 * Loads the good frames data from the stored workspace object
 */
void SinglePeriodLoadMuonStrategy::loadGoodFrames() {
  // Overwrite existing log entry
  auto &run = m_workspace->mutableRun();
  run.removeProperty("goodfrm");

  NXInt goodframes = LoadMuonNexus3Helper::loadGoodFramesDataFromNexus(
      m_entry, m_isFileMultiPeriod);

  if (m_isFileMultiPeriod) {
    run.addProperty("goodfrm", goodframes[static_cast<int>(m_entryNumber - 1)]);
  } else {
    run.addProperty("goodfrm", goodframes[0]);
  }
}

/**
 * Loads detector grouping.
 * If no entry in NeXus file for grouping, load it from the IDF.
 * stored
 * @returns :: Grouping table
 */
Workspace_sptr SinglePeriodLoadMuonStrategy::loadDetectorGrouping() const {

  auto grouping = LoadMuonNexus3Helper::loadDetectorGroupingFromNexus(
      m_entry, m_detectors, m_isFileMultiPeriod);
  DataObjects::TableWorkspace_sptr table =
      createDetectorGroupingTable(m_detectors, grouping);

  Workspace_sptr table_workspace;
  if (table->rowCount() != 0) {
    table_workspace = std::dynamic_pointer_cast<Workspace>(table);
  } else {
    m_logger.notice("Loading grouping information from IDF");
    table_workspace = loadDefaultDetectorGrouping();
  }
  return table_workspace;
}
/**
 * Loads default detector grouping, if this isn't present
 * return dummy grouping
 * If no entry in NeXus file for grouping, load it from the IDF.
 * @returns :: Grouping table
 */
Workspace_sptr
SinglePeriodLoadMuonStrategy::loadDefaultDetectorGrouping() const {

  auto instrument = m_workspace->getInstrument();
  auto &run = m_workspace->mutableRun();
  std::string mainFieldDirection =
      run.getLogData("main_field_direction")->value();
  API::GroupingLoader groupLoader(instrument, mainFieldDirection);
  try {
    const auto idfGrouping = groupLoader.getGroupingFromIDF();
    return idfGrouping->toTable();
  } catch (const std::runtime_error &) {
    auto dummyGrouping = std::make_shared<Grouping>();
    if (instrument->getNumberDetectors() != 0) {
      dummyGrouping = groupLoader.getDummyGrouping();
    } else {
      // Make sure it uses the right number of detectors
      std::ostringstream oss;
      oss << "1-" << m_workspace->getNumberHistograms();
      dummyGrouping->groups.emplace_back(oss.str());
      dummyGrouping->groupNames.emplace_back("all");
    }
    return dummyGrouping->toTable();
  }
}
/**
 * Loads dead time table, if this isn't present
 * @returns :: Dead time table
 */
Workspace_sptr SinglePeriodLoadMuonStrategy::loadDeadTimeTable() const {

  auto deadTimes = LoadMuonNexus3Helper::loadDeadTimesFromNexus(
      m_entry, m_detectors, m_isFileMultiPeriod);
  auto deadTimeTable = createDeadTimeTable(m_detectors, deadTimes);

  Workspace_sptr deadtimeWorkspace =
      std::dynamic_pointer_cast<Workspace>(deadTimeTable);

  return deadtimeWorkspace;
}
/**
 * Performs time-zero correction on the loaded workspace and also changes the
 * unit label on the time axis, which is incorrect due to being loaded using
 * LoadISISNexus2
 */
void SinglePeriodLoadMuonStrategy::applyTimeZeroCorrection() {

  double timeZero = LoadMuonNexus3Helper::loadTimeZeroFromNexusFile(m_entry);
  auto newUnit = std::dynamic_pointer_cast<Kernel::Units::Label>(
      Kernel::UnitFactory::Instance().create("Label"));
  newUnit->setLabel("Time", Kernel::Units::Symbol::Microsecond);
  m_workspace->getAxis(0)->unit() = newUnit;
  int numHistograms = static_cast<int>(m_workspace->getNumberHistograms());

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < numHistograms; i++) {
    auto &timeAxis = m_workspace->mutableX(i);
    timeAxis = timeAxis - timeZero;
  }
}
/**
 * Finds the detectors loaded into the present workspace
 */
std::vector<detid_t> SinglePeriodLoadMuonStrategy::getLoadedDetectors() {

  std::vector<detid_t> loadedDetectors;
  size_t numberOfSpectra = m_workspace->getNumberHistograms();

  for (size_t spectraIndex = 0; spectraIndex < numberOfSpectra;
       spectraIndex++) {
    const auto detIdSet =
        m_workspace->getSpectrum(spectraIndex).getDetectorIDs();
    // each spectrum should only point to one detector in the Muon file
    loadedDetectors.emplace_back(*detIdSet.begin());
  }
  return loadedDetectors;
}

} // namespace DataHandling
} // namespace Mantid
