// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonNexus3.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidDataHandling/LoadMuonNexus3Helper.h"
#include "MantidDataHandling/SinglePeriodLoadMuonStrategy.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"

#include <vector>

namespace Mantid {
namespace DataHandling {

DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadMuonNexus3)

using namespace Kernel;
using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;
using namespace DataObjects;

/// Empty default constructor
LoadMuonNexus3::LoadMuonNexus3()
    : m_filename(), m_sampleName(), m_isFileMultiPeriod(false),
      m_multiPeriodsLoaded(false) {}

/**
 * Return the confidence criteria for this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMuonNexus3::confidence(NexusHDF5Descriptor &descriptor) const {
  // Without this entry we cannot use LoadISISNexus
  if (!descriptor.isEntry("/raw_data_1", "NXentry")) {
    return 0;
  }
  const std::string root = "/raw_data_1";
  // Check if Muon source in defintiion entry
  if (!descriptor.isEntry(root + "/definition"))
    return 0;
  ::NeXus::File file(descriptor.getFilename());
  file.openPath(root + "/definition");
  std::string def = file.getStrData();
  if (def == "muonTD" || def == "pulsedTD") {
    return 82; // have to return 82 to "beat" the LoadMuonNexusV2 algorithm,
               // which returns 81 for this file as well
  } else {
    return 0;
  }
}
/// Initialization method.
void LoadMuonNexus3::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "",
                                                 FileProperty::Load, ".nxs"),
                  "The name of the Nexus file to load");

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "",
                                                     Direction::Output),
      "The name of the workspace to be created as the output of the\n"
      "algorithm. For multiperiod files, one workspace will be\n"
      "generated for each period");

  auto mustBePositive = std::make_shared<BoundedValidator<int64_t>>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumMin", static_cast<int64_t>(0), mustBePositive);
  declareProperty("SpectrumMax", static_cast<int64_t>(EMPTY_INT()),
                  mustBePositive);
  declareProperty(std::make_unique<ArrayProperty<int64_t>>("SpectrumList"));
  declareProperty("EntryNumber", static_cast<int64_t>(0), mustBePositive,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one worskspace");

  std::vector<std::string> FieldOptions{"Transverse", "Longitudinal"};
  declareProperty("MainFieldDirection", "Transverse",
                  std::make_shared<StringListValidator>(FieldOptions),
                  "Output the main field direction if specified in Nexus file "
                  "(run/instrument/detector/orientation, default "
                  "longitudinal). Version 1 only.",
                  Direction::Output);

  declareProperty("TimeZero", 0.0,
                  "Time zero in units of micro-seconds (default to 0.0)",
                  Direction::Output);
  declareProperty("FirstGoodData", 0.0,
                  "First good data in units of micro-seconds (default to 0.0)",
                  Direction::Output);

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>(
          "DeadTimeTable", "", Direction::Output, PropertyMode::Optional),
      "Table or a group of tables containing detector dead times. Version 1 "
      "only.");

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("DetectorGroupingTable",
                                                     "", Direction::Output,
                                                     PropertyMode::Optional),
      "Table or a group of tables with information about the "
      "detector grouping stored in the file (if any). Version 1 only.");
}
void LoadMuonNexus3::execLoader() {

  // prepare nexus entry
  m_entrynumber = getProperty("EntryNumber");
  m_filename = getPropertyValue("Filename");
  NXRoot root(m_filename);
  NXEntry entry = root.openEntry("raw_data_1");

  // Execute child algorithm LoadISISNexus2 and load Muon specific properties
  runLoadISISNexus();
  loadMuonProperties(entry);

  // Check if single or multi period file and create appriate loading strategy
  isEntryMultiPeriod(entry);
  Workspace_sptr outWS = getProperty("OutputWorkspace");
  if (m_multiPeriodsLoaded) {
    WorkspaceGroup_sptr wksp_grp =
        std::dynamic_pointer_cast<WorkspaceGroup>(outWS);
  } else {
    // we just have a single workspace
    Workspace2D_sptr workspace2D =
        std::dynamic_pointer_cast<Workspace2D>(outWS);
    m_loadMuonStrategy = std::make_unique<SinglePeriodLoadMuonStrategy>(
        g_log, m_filename, entry, workspace2D, static_cast<int>(m_entrynumber),
        m_isFileMultiPeriod);
  }
  m_loadMuonStrategy->loadMuonLogData();
  m_loadMuonStrategy->loadGoodFrames();
  m_loadMuonStrategy->applyTimeZeroCorrection();
  // Grouping info should be returned if user has set the property
  auto loadedGrouping = m_loadMuonStrategy->loadDetectorGrouping();
  if (!getPropertyValue("DetectorGroupingTable").empty()) {
    setProperty("DetectorGroupingTable", loadedGrouping);
  };
  // Deadtime table should be returned if user has set the property
  auto deadtimeTable = m_loadMuonStrategy->loadDeadTimeTable();
  if (!getPropertyValue("DeadTimeTable").empty()) {
    setProperty("DeadTimeTable", deadtimeTable);
  }
}

/**
 * Determines whether the file is multi period
 * If multi period the function determines whether multi periods are loaded
 */
void LoadMuonNexus3::isEntryMultiPeriod(const NXEntry &entry) {
  NXClass periodClass = entry.openNXGroup("periods");
  int numberOfPeriods = periodClass.getInt("number");
  if (numberOfPeriods > 1) {
    m_isFileMultiPeriod = true;
    if (m_entrynumber == 0) {
      m_multiPeriodsLoaded = true;
    }
  } else {
    m_isFileMultiPeriod = false;
    m_multiPeriodsLoaded = false;
  }
}
/**
 * Runs the child algorithm LoadISISNexus, which loads data into an output
 * workspace
 */
void LoadMuonNexus3::runLoadISISNexus() {
  IAlgorithm_sptr childAlg =
      createChildAlgorithm("LoadISISNexus", 0, 1, true, 2);
  declareProperty("LoadMonitors", "Exclude"); // we need to set this property
  auto ISISLoader = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
  ISISLoader->copyPropertiesFrom(*this);
  ISISLoader->executeAsChildAlg();
  this->copyPropertiesFrom(*ISISLoader);
}
/**
 * Loads Muon specific data from the nexus entry
 * and sets the appropriate output properties
 */
void LoadMuonNexus3::loadMuonProperties(const NXEntry &entry) {

  std::string mainFieldDirection =
      LoadMuonNexus3Helper::loadMainFieldDirectionFromNexus(entry);
  setProperty("MainFieldDirection", mainFieldDirection);

  double timeZero = LoadMuonNexus3Helper::loadTimeZeroFromNexusFile(entry);
  setProperty("timeZero", timeZero);

  auto firstGoodData = LoadMuonNexus3Helper::loadFirstGoodDataFromNexus(entry);
  setProperty("FirstGoodData", firstGoodData);
}
} // namespace DataHandling
} // namespace Mantid
