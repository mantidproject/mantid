// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SinglePeriodLoadMuonStrategy.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidDataHandling/LoadMuonLog.h"
#include "MantidDataHandling/LoadMuonNexusV2Helper.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;
using namespace DataObjects;

namespace NeXusEntry {
const std::string SAMPLE{"sample"};
const std::string TEMPERATURE{"temperature"};
const std::string MAGNETICFIELD{"magnetic_field"};
} // namespace NeXusEntry

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
  auto runSample = m_entry.openNXGroup(NeXusEntry::SAMPLE);

  if (runSample.containsDataSet(NeXusEntry::TEMPERATURE)) {
    float temperature = runSample.getFloat(NeXusEntry::TEMPERATURE);
    run.addProperty("sample_temp", static_cast<double>(temperature));
  }

  if (runSample.containsDataSet(NeXusEntry::MAGNETICFIELD)) {
    float magn_field = runSample.getFloat(NeXusEntry::MAGNETICFIELD);
    run.addProperty("sample_magn_field", static_cast<double>(magn_field));
  }
  std::string mainFieldDirection =
      LoadMuonNexusV2Helper::loadMainFieldDirectionFromNexus(m_entry);
  run.addProperty("main_field_direction", mainFieldDirection);

  double firstGoodData =
      LoadMuonNexusV2Helper::loadFirstGoodDataFromNexus(m_entry);
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
    run.addProperty(goodframeProp,
                    goodframes[static_cast<int>(m_entryNumber - 1)]);
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
      m_entry, m_detectors, m_isFileMultiPeriod);
  TableWorkspace_sptr table =
      createDetectorGroupingTable(m_detectors, grouping);

  if (table->rowCount() != 0) {
    return table;
  } else {
    m_logger.notice("Loading grouping information from IDF");
    return loadDefaultDetectorGrouping();
  }
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
  auto deadTimes = LoadMuonNexusV2Helper::loadDeadTimesFromNexus(
      m_entry, m_detectors, m_isFileMultiPeriod);
  return createDeadTimeTable(m_detectors, deadTimes);
}
/**
 * Performs time-zero correction on the loaded workspace and also changes the
 * unit label on the time axis, which is incorrect due to being loaded using
 * LoadISISNexus2
 */
void SinglePeriodLoadMuonStrategy::applyTimeZeroCorrection() {
  double timeZero = LoadMuonNexusV2Helper::loadTimeZeroFromNexusFile(m_entry);
  auto newUnit = std::dynamic_pointer_cast<Kernel::Units::Label>(
      Kernel::UnitFactory::Instance().create("Label"));
  newUnit->setLabel("Time", Kernel::Units::Symbol::Microsecond);
  m_workspace->getAxis(0)->unit() = newUnit;

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
