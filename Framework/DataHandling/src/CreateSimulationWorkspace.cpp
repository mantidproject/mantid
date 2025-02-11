// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/CreateSimulationWorkspace.h"
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataHandling/StartAndEndTimeFromNexusFileExtractor.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

namespace {

struct StartAndEndTime {
  Mantid::Types::Core::DateAndTime startTime;
  Mantid::Types::Core::DateAndTime endTime;
};

StartAndEndTime getStartAndEndTimesFromRawFile(const std::string &filename) {
  FILE *rawFile = fopen(filename.c_str(), "rb");
  if (!rawFile)
    throw std::runtime_error("Cannot open RAW file for reading: " + filename);

  ISISRAW2 isisRaw;
  const bool fromFile(true), readData(false);
  isisRaw.ioRAW(rawFile, fromFile, readData);

  StartAndEndTime startAndEndTime;
  startAndEndTime.startTime = Mantid::DataHandling::LoadRawHelper::extractStartTime(isisRaw);
  startAndEndTime.endTime = Mantid::DataHandling::LoadRawHelper::extractEndTime(isisRaw);

  fclose(rawFile);
  return startAndEndTime;
}

StartAndEndTime getStartAndEndTimesFromNexusFile(const std::string &filename,
                                                 const Mantid::Types::Core::DateAndTime &startTimeDefault,
                                                 const Mantid::Types::Core::DateAndTime &endTimeDefault) {
  StartAndEndTime startAndEndTime;
  try {
    startAndEndTime.startTime = Mantid::DataHandling::extractStartTime(filename);
    startAndEndTime.endTime = Mantid::DataHandling::extractEndTime(filename);
  } catch (...) {
    startAndEndTime.startTime = startTimeDefault;
    startAndEndTime.endTime = endTimeDefault;
  }

  return startAndEndTime;
}
} // namespace

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateSimulationWorkspace)

using namespace API;
using namespace HistogramData;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateSimulationWorkspace::name() const { return "CreateSimulationWorkspace"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateSimulationWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateSimulationWorkspace::category() const { return "Utility\\Workspaces"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateSimulationWorkspace::init() {
  using namespace Kernel;

  declareProperty("Instrument", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "An instrument name or filename ( a full path or string "
                  "containing an xml extension).",
                  Direction::Input);

  declareProperty(
      std::make_unique<ArrayProperty<double>>("BinParams", std::make_shared<RebinParamsValidator>(), Direction::Input),
      "A comma separated list of first bin boundary, width, last "
      "bin boundary. See Rebin for more details");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output), "The new workspace");

  auto knownUnits = UnitFactory::Instance().getKeys();
  declareProperty("UnitX", "DeltaE", std::make_shared<ListValidator<std::string>>(knownUnits),
                  "The unit to assign to the X axis", Direction::Input);

  declareProperty(
      std::make_unique<FileProperty>("DetectorTableFilename", "", FileProperty::OptionalLoad, "", Direction::Input),
      "An optional filename (currently RAW or ISIS NeXus) that "
      "contains UDET & SPEC tables to access hardware grouping");

  declareProperty("SetErrors", false, "Whether to set histogram bin errors to sqrt of intensity.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateSimulationWorkspace::exec() {
  createInstrument();
  createOutputWorkspace();

  setProperty("OutputWorkspace", m_outputWS);
}

//-----------------------------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------------------------
/**
 * Create the instrument from the name/file. Runs LoadInstrument with a fake
 * workspace.
 */
void CreateSimulationWorkspace::createInstrument() {
  const bool enableLogging(false);
  auto loadInstrument = createChildAlgorithm("LoadInstrument", 0.0, 0.5, enableLogging);
  MatrixWorkspace_sptr tempWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

  // We need to set the correct start date for this workspace
  // else we might be pulling an inadequate IDF
  setStartDate(tempWS);

  loadInstrument->setProperty("Workspace", tempWS);
  const std::string instrProp = getProperty("Instrument");
  if (instrProp.ends_with(".xml")) {
    loadInstrument->setPropertyValue("Filename", instrProp);
  } else {
    loadInstrument->setPropertyValue("InstrumentName", instrProp);
  }
  loadInstrument->setProperty("RewriteSpectraMap", Kernel::OptionalBool(true));
  loadInstrument->executeAsChildAlg();
  tempWS = loadInstrument->getProperty("Workspace");

  m_instrument = tempWS->getInstrument();
}

/**
 * Creates the output workspace attaching the instrument
 */
void CreateSimulationWorkspace::createOutputWorkspace() {
  const size_t nhistograms = createDetectorMapping();
  const auto binBoundaries = createBinBoundaries();
  const size_t xlength = binBoundaries.size();
  const size_t ylength = xlength - 1;
  const bool setError = getProperty("SetErrors");

  m_outputWS = WorkspaceFactory::Instance().create("Workspace2D", nhistograms, xlength, ylength);
  m_outputWS->setInstrument(m_instrument);
  m_outputWS->populateInstrumentParameters();

  m_outputWS->getAxis(0)->setUnit(getProperty("UnitX"));
  m_outputWS->setYUnit("SpectraNumber");

  m_progress = std::make_shared<Progress>(this, 0.5, 0.75, nhistograms);

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_outputWS))
  for (int64_t i = 0; i < static_cast<int64_t>(nhistograms); ++i) {
    m_outputWS->setBinEdges(i, binBoundaries);
    m_outputWS->mutableY(i) = 1.0;
    if (setError) {
      m_outputWS->mutableE(i).assign(m_outputWS->getNumberBins(i), sqrt(1.0));
    }

    m_progress->report("Setting X values");
  }
  applyDetectorMapping();

  // Update the instrument from the file if necessary
  const std::string detTableFile = getProperty("DetectorTableFilename");
  if (detTableFile.ends_with(".raw") || detTableFile.ends_with(".RAW") || detTableFile.ends_with(".nxs") ||
      detTableFile.ends_with(".NXS")) {
    adjustInstrument(detTableFile);
  }
}

/**
 * Sets up the detector map. By default a 1:1 map is ensured, however a file can
 * be given to use as a map
 * @returns The number of spectra that are required
 */
size_t CreateSimulationWorkspace::createDetectorMapping() {
  const std::string detTableFile = getProperty("DetectorTableFilename");
  if (detTableFile.empty()) {
    createOneToOneMapping();
  } else {
    loadMappingFromFile(detTableFile);
  }
  return m_detGroups.size();
}

/**
 * Create a one to one mapping from the spectrum numbers to detector IDs
 */
void CreateSimulationWorkspace::createOneToOneMapping() {
  const std::vector<detid_t> detids = m_instrument->getDetectorIDs(true);
  const size_t nhist = detids.size();

  m_detGroups.clear();
  for (size_t i = 0; i < nhist; ++i) {
    std::set<detid_t> group;
    group.insert(detids[i]);
    m_detGroups.emplace(static_cast<specnum_t>(i + 1), group);
  }
}

/**
 * Load the detector mapping from a file
 * @param filename :: The name of the file to pull the UDET/SPEC tables from
 */
void CreateSimulationWorkspace::loadMappingFromFile(const std::string &filename) {
  if (filename.ends_with(".raw") || filename.ends_with(".RAW")) {
    loadMappingFromRAW(filename);
  } else if (filename.ends_with(".nxs") || filename.ends_with(".NXS")) {
    loadMappingFromISISNXS(filename);
  }
}

/**
 * Load the detector mapping from a RAW file
 * @param filename :: The name of the RAW file to pull the UDET/SPEC tables from
 */
void CreateSimulationWorkspace::loadMappingFromRAW(const std::string &filename) {
  FILE *rawFile = fopen(filename.c_str(), "rb");
  if (!rawFile)
    throw std::runtime_error("Cannot open RAW file for reading: " + filename);

  ISISRAW2 isisRaw;
  const bool fromFile(true), readData(false);
  isisRaw.ioRAW(rawFile, fromFile, readData);

  int ndet = isisRaw.i_det;
  const int *specTable = isisRaw.spec;
  const int *udetTable = isisRaw.udet;
  createGroupingsFromTables(specTable, udetTable, ndet);

  fclose(rawFile);
}

/**
 * Load the detector mapping from a NeXus file. Throws if the file does not
 * provide the mapping tables
 * @param filename :: The name of the ISIS raw NeXus file to pull the UDET/SPEC
 * tables from
 */
void CreateSimulationWorkspace::loadMappingFromISISNXS(const std::string &filename) {
  ::NeXus::File nxsFile(filename);
  try {
    nxsFile.openPath("/raw_data_1/isis_vms_compat");
  } catch (::NeXus::Exception &) {
    throw std::runtime_error("Cannot find path to isis_vms_compat. Is the file an ISIS NeXus file?");
  }
  using NXIntArray = std::vector<int32_t>;

  NXIntArray ndets;
  nxsFile.readData("NDET", ndets);

  NXIntArray specTable;
  nxsFile.readData("SPEC", specTable);

  NXIntArray udetTable;
  nxsFile.readData("UDET", udetTable);

  createGroupingsFromTables(specTable.data(), udetTable.data(), ndets[0]);
}

/**
 * Create the grouping map from the tables
 * @param specTable :: An array of spectrum numbers
 * @param udetTable :: An array of detector IDs
 * @param ndets :: The size of the two arrays
 */
void CreateSimulationWorkspace::createGroupingsFromTables(const int *specTable, const int *udetTable, int ndets) {
  m_detGroups.clear();
  for (int i = 0; i < ndets; ++i) {
    int specNo = specTable[i];
    int detID = udetTable[i];
    if (m_instrument->isMonitor(detID))
      continue; // Skip monitors

    auto iter = m_detGroups.find(specNo);
    if (iter != m_detGroups.end()) {
      iter->second.insert(detID);
    } else {
      std::set<detid_t> group;
      group.insert(static_cast<detid_t>(detID));
      m_detGroups.emplace(specNo, group);
    }
  }
}

/**
 * @returns The bin bounadries for the new workspace
 */
BinEdges CreateSimulationWorkspace::createBinBoundaries() const {
  const std::vector<double> rbparams = getProperty("BinParams");
  MantidVec newBins;
  const int numBoundaries = Mantid::Kernel::VectorHelper::createAxisFromRebinParams(rbparams, newBins);
  if (numBoundaries <= 2) {
    throw std::invalid_argument("Error in BinParams - Gave invalid number of bin boundaries: " +
                                std::to_string(numBoundaries));
  }
  return BinEdges(std::move(newBins));
}

/**
 * Apply the created mapping to the workspace
 */
void CreateSimulationWorkspace::applyDetectorMapping() {
  size_t wsIndex(0);
  for (auto &detGroup : m_detGroups) {
    auto &spectrum = m_outputWS->getSpectrum(wsIndex);
    spectrum.setSpectrumNo(static_cast<specnum_t>(wsIndex + 1)); // Ensure a contiguous mapping
    spectrum.clearDetectorIDs();
    spectrum.addDetectorIDs(detGroup.second);
    ++wsIndex;
  }
}

/**
 * Apply any instrument adjustments from the file
 * @param filename :: The file to take the positions
 */
void CreateSimulationWorkspace::adjustInstrument(const std::string &filename) {
  // If requested update the instrument to positions in the raw file
  const auto &pmap = m_outputWS->constInstrumentParameters();
  Geometry::Instrument_const_sptr instrument = m_outputWS->getInstrument();
  std::shared_ptr<Geometry::Parameter> updateDets = pmap.get(instrument->getComponentID(), "det-pos-source");
  if (!updateDets)
    return; // No tag, use IDF

  std::string value = updateDets->value<std::string>();
  if (value.substr(0, 8) == "datafile") {
    auto updateInst = createChildAlgorithm("UpdateInstrumentFromFile", 0.75, 1.0);
    updateInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_outputWS);
    updateInst->setPropertyValue("Filename", filename);
    if (value == "datafile-ignore-phi") {
      updateInst->setProperty("IgnorePhi", true);
      g_log.information("Detector positions in IDF updated with positions in "
                        "the data file except for the phi values");
    } else {
      g_log.information("Detector positions in IDF updated with positions in the data file");
    }
    // We want this to throw if it fails to warn the user that the information
    // is not correct.
    updateInst->execute();
  }
}

/**
 * Sets the start date on a dummy workspace. If there is a detector table file
 * available we update the dummy workspace with the start date from this file.
 * @param workspace: dummy workspace
 */
void CreateSimulationWorkspace::setStartDate(const API::MatrixWorkspace_sptr &workspace) {
  const std::string detTableFile = getProperty("DetectorTableFilename");
  auto hasDetTableFile = !detTableFile.empty();
  auto &run = workspace->mutableRun();

  Types::Core::DateAndTime startTime;
  Types::Core::DateAndTime endTime;
  try {
    // The start and end times might not be valid, and hence can throw
    startTime = run.startTime();
    endTime = run.endTime();
  } catch (std::runtime_error &) {
    startTime = Types::Core::DateAndTime::getCurrentTime();
    endTime = Types::Core::DateAndTime::getCurrentTime();
  }

  if (hasDetTableFile) {
    if (detTableFile.ends_with(".raw") || detTableFile.ends_with(".RAW")) {
      auto startAndEndTime = getStartAndEndTimesFromRawFile(detTableFile);
      startTime = startAndEndTime.startTime;
      endTime = startAndEndTime.endTime;
    } else if (detTableFile.ends_with(".nxs") || detTableFile.ends_with(".NXS")) {
      auto startAndEndTime = getStartAndEndTimesFromNexusFile(detTableFile, startTime, endTime);
      startTime = startAndEndTime.startTime;
      endTime = startAndEndTime.endTime;
    }
  }

  run.setStartAndEndTime(startTime, endTime);
}

} // namespace Mantid::DataHandling
