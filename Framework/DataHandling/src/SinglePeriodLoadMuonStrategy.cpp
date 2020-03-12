
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SinglePeriodLoadMuonStrategy.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/ISISRunLogs.h"
#include "MantidDataHandling/LoadMuonLog.h"
#include "MantidDataHandling/LoadMuonNexus3Helper.h"

#include <iostream>

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
      m_entryNumber(entryNumber), m_isFileMultiPeriod(isFileMultiPeriod) {}

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
 * Loads the good frames assuming we have just one workavoid if else in c++space
 * loaded for a workspace 2d object
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
Workspace_sptr SinglePeriodLoadMuonStrategy::loadDetectorGrouping() {

  TableWorkspace_sptr table =
      LoadMuonNexus3Helper::loadDetectorGroupingFromNexus(m_entry, m_workspace,
                                                          m_isFileMultiPeriod);
  Workspace_sptr table_workspace;
  if (table->rowCount() != 0) {
    table_workspace = boost::dynamic_pointer_cast<Workspace>(table);
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
    auto dummyGrouping = boost::make_shared<Grouping>();
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
void SinglePeriodLoadMuonStrategy::loadDeadTimeTable() const {

  int b = 3;
  int c = 3;
  std::cout << "LOADING DEAD TIME TABLE" << std::endl;
}

} // namespace DataHandling
} // namespace Mantid
