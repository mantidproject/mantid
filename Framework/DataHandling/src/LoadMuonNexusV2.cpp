// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonNexusV2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidDataHandling/MultiPeriodLoadMuonStrategy.h"
#include "MantidDataHandling/SinglePeriodLoadMuonStrategy.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <vector>

namespace Mantid::DataHandling {

DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadMuonNexusV2)

using namespace Kernel;
using namespace API;
using namespace NeXus;
using namespace HistogramData;
using std::size_t;
using namespace DataObjects;

namespace NeXusEntry {
const std::string RAWDATA{"/raw_data_1"};
const std::string DEFINITION{"/raw_data_1/definition"};
const std::string BEAMLINE{"/raw_data_1/beamline"};
} // namespace NeXusEntry

/// Empty default constructor
LoadMuonNexusV2::LoadMuonNexusV2()
    : m_filename(), m_entrynumber(0), m_isFileMultiPeriod(false), m_multiPeriodsLoaded(false) {}

/**
 * Return the confidence criteria for this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMuonNexusV2::confidence(NexusHDF5Descriptor &descriptor) const {
  // Without this entry we cannot use LoadISISNexus
  if (!descriptor.isEntry(NeXusEntry::RAWDATA, "NXentry")) {
    return 0;
  }

  // Check if beamline entry exists beneath raw_data_1 - /raw_data_1/beamline
  // Necessary to differentiate between ISIS and PSI nexus files.
  if (!descriptor.isEntry(NeXusEntry::BEAMLINE))
    return 0;

  // Check if Muon source in definition entry
  if (!descriptor.isEntry(NeXusEntry::DEFINITION))
    return 0;

  ::NeXus::File file(descriptor.filename());
  file.openPath(NeXusEntry::DEFINITION);
  std::string def = file.getStrData();
  if (def == "muonTD" || def == "pulsedTD") {
    return 82; // have to return 82 to "beat" the LoadMuonNexus2 algorithm,
               // which returns 81 for this file as well
  } else {
    return 0;
  }
}
/// Initialization method.
void LoadMuonNexusV2::init() {

  std::vector<std::string> extensions{".nxs", ".nxs_v2", ".nxs_v1"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, extensions),
                  "The name of the Nexus file to load");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the\n"
                  "algorithm. For multiperiod files, one workspace will be\n"
                  "generated for each period");

  auto mustBePositiveSpectra = std::make_shared<BoundedValidator<specnum_t>>();
  mustBePositiveSpectra->setLower(0);
  declareProperty("SpectrumMin", static_cast<specnum_t>(0), mustBePositiveSpectra);
  declareProperty("SpectrumMax", static_cast<specnum_t>(EMPTY_INT()), mustBePositiveSpectra);
  declareProperty(std::make_unique<ArrayProperty<specnum_t>>("SpectrumList"));
  auto mustBePositive = std::make_shared<BoundedValidator<int64_t>>();
  mustBePositive->setLower(0);
  declareProperty("EntryNumber", static_cast<int64_t>(0), mustBePositive,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one workspace");

  std::vector<std::string> FieldOptions{"Transverse", "Longitudinal"};
  declareProperty("MainFieldDirection", "Transverse", std::make_shared<StringListValidator>(FieldOptions),
                  "Output the main field direction if specified in Nexus file "
                  "(default longitudinal).",
                  Direction::Output);

  declareProperty("TimeZero", 0.0, "Time zero in units of micro-seconds (default to 0.0)", Direction::Output);
  declareProperty("FirstGoodData", 0.0, "First good data in units of micro-seconds (default to 0.0)",
                  Direction::Output);
  declareProperty("LastGoodData", 0.0, "Last good data in the OutputWorkspace's spectra", Kernel::Direction::Output);

  declareProperty(std::make_unique<ArrayProperty<double>>("TimeZeroList", Direction::Output),
                  "A vector of time zero values");

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("TimeZeroTable", "", Direction::Output, PropertyMode::Optional),
      "TableWorkspace containing time zero values per spectra.");

  declareProperty("CorrectTime", true, "Boolean flag controlling whether time should be corrected by timezero.",
                  Direction::Input);

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("DeadTimeTable", "", Direction::Output, PropertyMode::Optional),
      "Table or a group of tables containing detector dead times.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("DetectorGroupingTable", "", Direction::Output,
                                                                 PropertyMode::Optional),
                  "Table or a group of tables with information about the "
                  "detector grouping.");
}
void LoadMuonNexusV2::execLoader() {
  // prepare nexus entry
  m_entrynumber = getProperty("EntryNumber");
  m_filename = getPropertyValue("Filename");
  NXRoot root(m_filename);
  NXEntry entry = root.openEntry(NeXusEntry::RAWDATA);
  // Create MuonNexusV2 nexus loader
  m_nexusLoader = std::make_unique<LoadMuonNexusV2NexusHelper>(entry);
  isEntryMultiPeriod();

  // Execute child algorithm LoadISISNexus2
  auto outWS = runLoadISISNexus();
  // Create appropriate loading strategy
  chooseLoaderStrategy(outWS);
  m_loadMuonStrategy->loadMuonLogData();
  m_loadMuonStrategy->loadGoodFrames();
  auto correctTime = getProperty("CorrectTime");
  if (correctTime) {
    m_loadMuonStrategy->applyTimeZeroCorrection();
  }
  // Grouping info should be returned if user has set the property
  if (!getPropertyValue("DetectorGroupingTable").empty()) {
    auto loadedGrouping = m_loadMuonStrategy->loadDetectorGrouping();
    setProperty("DetectorGroupingTable", loadedGrouping);
  };
  // Deadtime table should be returned if user has set the property
  auto deadtimeTable = m_loadMuonStrategy->loadDeadTimeTable();
  if (!getPropertyValue("DeadTimeTable").empty()) {
    setProperty("DeadTimeTable", deadtimeTable);
  }

  // Time Zero table should be returned if found
  if (!getPropertyValue("TimeZerotable").empty()) {
    // Create table and set property
    auto timeZeroTable = m_loadMuonStrategy->getTimeZeroTable();
    setProperty("TimeZeroTable", timeZeroTable);
  }
}

/**
 * Determines whether the file is multi period
 * If multi period the function determines whether multi periods are loaded
 */
void LoadMuonNexusV2::isEntryMultiPeriod() {
  int numberOfPeriods = m_nexusLoader->getNumberOfPeriods();
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
 * @returns :: Workspace loaded by runLoadISISNexus
 */
Workspace_sptr LoadMuonNexusV2::runLoadISISNexus() {
  // Here we explicit set the number of OpenMP threads, as by default
  // LoadISISNexus spawns up a large number of threads,
  // which is unnecessary for the size (~100 spectra) of workspaces seen here.
  // Through profiling it was found that a single threaded call to LoadISISNexus
  // was quicker due to the overhead of setting up the threads, which outweighs
  // the cost of the resulting operations.
  // To prevent the omp_set_num_threads call having side effects, we use a RAII
  // pattern to restore the default behavior once runLoadISISNexus is complete.
  struct ScopedNumThreadsSetter {
    ScopedNumThreadsSetter(const int numThreads) {
      (void)numThreads; // Treat compiler warning in OSX
      globalNumberOfThreads = PARALLEL_GET_MAX_THREADS;
      PARALLEL_SET_NUM_THREADS(numThreads);
    }
    ~ScopedNumThreadsSetter() { PARALLEL_SET_NUM_THREADS(globalNumberOfThreads); }
    int globalNumberOfThreads;
  };
  ScopedNumThreadsSetter restoreDefaultThreadsOnExit(1);
  auto childAlg = createChildAlgorithm("LoadISISNexus", 0, 1, true, 2);
  declareProperty("LoadMonitors", "Exclude"); // we need to set this property
  auto ISISLoader = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
  ISISLoader->copyPropertiesFrom(*this);
  ISISLoader->execute();
  this->copyPropertiesFrom(*ISISLoader);
  Workspace_sptr outWS = getProperty("OutputWorkspace");
  applyTimeAxisUnitCorrection(*outWS);
  loadPeriodInfo(*outWS);
  return outWS;
}
/**
 * Determines the loading strategy used by the Algorithm
 */
void LoadMuonNexusV2::chooseLoaderStrategy(const Workspace_sptr &workspace) {
  // Check if single or multi period file and create appropriate loading
  // strategy
  if (m_multiPeriodsLoaded) {
    WorkspaceGroup_sptr workspaceGroup = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    assert(workspaceGroup);
    auto numberHistograms = std::dynamic_pointer_cast<Workspace2D>(workspaceGroup->getItem(0))->getNumberHistograms();
    loadMuonProperties(numberHistograms);
    m_loadMuonStrategy =
        std::make_unique<MultiPeriodLoadMuonStrategy>(g_log, m_filename, *m_nexusLoader, *workspaceGroup);

  } else {
    // we just have a single workspace
    Workspace2D_sptr workspace2D = std::dynamic_pointer_cast<Workspace2D>(workspace);
    assert(workspace2D);
    // Load Muon specific properties
    loadMuonProperties(workspace2D->getNumberHistograms());
    m_loadMuonStrategy = std::make_unique<SinglePeriodLoadMuonStrategy>(
        g_log, m_filename, *m_nexusLoader, *workspace2D, static_cast<int>(m_entrynumber), m_isFileMultiPeriod);
  }
}
/**
 * Loads Muon specific data from the nexus entry
 * and sets the appropriate output properties
 */
void LoadMuonNexusV2::loadMuonProperties(size_t numSpectra) {

  std::string mainFieldDirection = m_nexusLoader->loadMainFieldDirectionFromNexus();
  setProperty("MainFieldDirection", mainFieldDirection);

  double timeZero = m_nexusLoader->loadTimeZeroFromNexusFile();
  setProperty("timeZero", timeZero);

  auto firstGoodData = m_nexusLoader->loadFirstGoodDataFromNexus();
  setProperty("FirstGoodData", firstGoodData);

  auto lastGoodData = m_nexusLoader->loadLastGoodDataFromNexus();
  setProperty("LastGoodData", lastGoodData);

  auto timeZeroVector = m_nexusLoader->loadTimeZeroListFromNexusFile(numSpectra);
  setProperty("TimeZeroList", timeZeroVector);
}

/*
Changes the unit of the time axis, which is incorrect due to being loaded using
LoadISISNexus
*/
void LoadMuonNexusV2::applyTimeAxisUnitCorrection(Workspace &workspace) {
  auto newUnit = std::dynamic_pointer_cast<Kernel::Units::Label>(Kernel::UnitFactory::Instance().create("Label"));
  newUnit->setLabel("Time", Kernel::Units::Symbol::Microsecond);
  auto workspaceGroup = dynamic_cast<WorkspaceGroup *>(&workspace);
  if (workspaceGroup) {
    for (int i = 0; i < workspaceGroup->getNumberOfEntries(); ++i) {
      auto workspace2D = std::dynamic_pointer_cast<Workspace2D>(workspaceGroup->getItem(i));
      workspace2D->getAxis(0)->unit() = newUnit;
    }
  } else {
    auto &workspace2D = dynamic_cast<Workspace2D &>(workspace);
    workspace2D.getAxis(0)->unit() = newUnit;
  }
}
void LoadMuonNexusV2::loadPeriodInfo(Workspace &workspace) {
  // get value
  int numberOfPeriods = m_nexusLoader->getNumberOfPeriods();
  auto labels = m_nexusLoader->getPeriodLabels();
  auto sequences = m_nexusLoader->getPeriodSequenceString(numberOfPeriods);
  auto types = m_nexusLoader->getPeriodTypes(numberOfPeriods);
  auto requested = m_nexusLoader->getPeriodFramesRequested(numberOfPeriods);
  auto rawFrames = m_nexusLoader->getPeriodRawFrames(numberOfPeriods);
  auto output = m_nexusLoader->getPeriodOutput(numberOfPeriods);
  auto counts = m_nexusLoader->getPeriodTotalCounts();
  // put values into workspaces
  auto workspaceGroup = dynamic_cast<WorkspaceGroup *>(&workspace);
  if (workspaceGroup) {
    for (int i = 0; i < workspaceGroup->getNumberOfEntries(); ++i) {
      auto workspace2D = std::dynamic_pointer_cast<Workspace2D>(workspaceGroup->getItem(i));
      auto &run = workspace2D->mutableRun();
      run.addProperty("period_labels", labels);
      run.addProperty("period_sequences", sequences);
      run.addProperty("period_type", types);
      run.addProperty("frames_period_requested", requested);
      run.addProperty("frames_period_raw", rawFrames);
      run.addProperty("period_output", output);
      run.addProperty("total_counts_period", counts);
    }
  } else {
    auto &workspace2D = dynamic_cast<Workspace2D &>(workspace);
    auto &run = workspace2D.mutableRun();
    run.addProperty("period_labels", labels);
    run.addProperty("period_sequences", sequences);
    run.addProperty("period_type", types);
    run.addProperty("frames_period_requested", requested);
    run.addProperty("frames_period_raw", rawFrames);
    run.addProperty("period_output", output);
    run.addProperty("total_counts_period", counts);
  }
}

} // namespace Mantid::DataHandling
