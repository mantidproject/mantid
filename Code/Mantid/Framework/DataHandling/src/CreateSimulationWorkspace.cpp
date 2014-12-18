#include "MantidDataHandling/CreateSimulationWorkspace.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include "LoadRaw/isisraw2.h"
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include <Poco/File.h>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateSimulationWorkspace);

using namespace API;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateSimulationWorkspace::name() const {
  return "CreateSimulationWorkspace";
};

/// Algorithm's version for identification. @see Algorithm::version
int CreateSimulationWorkspace::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateSimulationWorkspace::category() const {
  return "Quantification";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateSimulationWorkspace::init() {
  using namespace Kernel;

  declareProperty("Instrument", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "An instrument name or filename ( a full path or string "
                  "containing an xml extension).",
                  Direction::Input);

  declareProperty(new ArrayProperty<double>(
                      "BinParams", boost::make_shared<RebinParamsValidator>(),
                      Direction::Input),
                  "A comma separated list of first bin boundary, width, last "
                  "bin boundary. See Rebin for more details");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The new workspace");

  auto knownUnits = UnitFactory::Instance().getKeys();
  declareProperty("UnitX", "DeltaE",
                  boost::make_shared<ListValidator<std::string>>(knownUnits),
                  "The unit to assign to the X axis", Direction::Input);

  declareProperty(new FileProperty("DetectorTableFilename", "",
                                   FileProperty::OptionalLoad, "",
                                   Direction::Input),
                  "An optional filename (currently RAW or ISIS NeXus) that "
                  "contains UDET & SPEC tables to access hardware grouping");
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
  IAlgorithm_sptr loadInstrument =
      createChildAlgorithm("LoadInstrument", 0.0, 0.5, enableLogging);
  MatrixWorkspace_sptr tempWS =
      WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  loadInstrument->setProperty("Workspace", tempWS);
  const std::string instrProp = getProperty("Instrument");
  if (boost::algorithm::ends_with(instrProp, ".xml")) {
    loadInstrument->setPropertyValue("Filename", instrProp);
  } else {
    loadInstrument->setPropertyValue("InstrumentName", instrProp);
  }
  loadInstrument->executeAsChildAlg();
  tempWS = loadInstrument->getProperty("Workspace");

  m_instrument = tempWS->getInstrument();
}

/**
 * Creates the output workspace attaching the instrument
 */
void CreateSimulationWorkspace::createOutputWorkspace() {
  const size_t nhistograms = createDetectorMapping();
  const MantidVecPtr binBoundaries = createBinBoundaries();
  const size_t xlength = binBoundaries->size();
  const size_t ylength = xlength - 1;

  m_outputWS = WorkspaceFactory::Instance().create("Workspace2D", nhistograms,
                                                   xlength, ylength);
  m_outputWS->setInstrument(m_instrument);
  m_outputWS->populateInstrumentParameters();

  m_outputWS->getAxis(0)->setUnit(getProperty("UnitX"));
  m_outputWS->setYUnit("SpectraNumber");

  m_progress =
      boost::shared_ptr<Progress>(new Progress(this, 0.5, 0.75, nhistograms));

  PARALLEL_FOR1(m_outputWS)
  for (int64_t i = 0; i < static_cast<int64_t>(nhistograms); ++i) {
    m_outputWS->setX(i, binBoundaries);
    MantidVec &yOut = m_outputWS->dataY(i);
    for (size_t j = 0; j < ylength; ++j) {
      yOut[j] = 1.0; // Set everything to a value so that you can visualize the
                     // output sensibly
    }
    m_progress->report("Setting X values");
  }
  applyDetectorMapping();

  // Update the instrument from the file if necessary
  const std::string detTableFile = getProperty("DetectorTableFilename");
  if (boost::algorithm::ends_with(detTableFile, ".raw") ||
      boost::algorithm::ends_with(detTableFile, ".RAW") ||
      boost::algorithm::ends_with(detTableFile, ".nxs") ||
      boost::algorithm::ends_with(detTableFile, ".NXS")) {
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
    m_detGroups.insert(std::make_pair(static_cast<specid_t>(i + 1), group));
  }
}

/**
 * Load the detector mapping from a file
 * @param filename :: The name of the file to pull the UDET/SPEC tables from
 */
void
CreateSimulationWorkspace::loadMappingFromFile(const std::string &filename) {
  if (boost::algorithm::ends_with(filename, ".raw") ||
      boost::algorithm::ends_with(filename, ".RAW")) {
    loadMappingFromRAW(filename);
  } else if (boost::algorithm::ends_with(filename, ".nxs") ||
             boost::algorithm::ends_with(filename, ".NXS")) {
    loadMappingFromISISNXS(filename);
  }
}

/**
 * Load the detector mapping from a RAW file
 * @param filename :: The name of the RAW file to pull the UDET/SPEC tables from
 */
void
CreateSimulationWorkspace::loadMappingFromRAW(const std::string &filename) {
  FILE *rawFile = fopen(filename.c_str(), "rb");
  if (!rawFile)
    throw std::runtime_error("Cannot open RAW file for reading: " + filename);

  ISISRAW2 isisRaw;
  const bool fromFile(true), readData(false);
  isisRaw.ioRAW(rawFile, fromFile, readData);

  int ndet = isisRaw.i_det;
  int *specTable = isisRaw.spec;
  int *udetTable = isisRaw.udet;
  createGroupingsFromTables(specTable, udetTable, ndet);

  fclose(rawFile);
}

/**
 * Load the detector mapping from a NeXus file. Throws if the file does not
 * provide the mapping tables
 * @param filename :: The name of the ISIS raw NeXus file to pull the UDET/SPEC
 * tables from
 */
void
CreateSimulationWorkspace::loadMappingFromISISNXS(const std::string &filename) {
  ::NeXus::File nxsFile(filename);
  try {
    nxsFile.openPath("/raw_data_1/isis_vms_compat");
  } catch (::NeXus::Exception &) {
    throw std::runtime_error(
        "Cannot find path to isis_vms_compat. Is the file an ISIS NeXus file?");
  }
  typedef boost::scoped_ptr<std::vector<int32_t>> NXIntArray;

  nxsFile.openData("NDET");
  NXIntArray ndets(nxsFile.getData<int32_t>());
  nxsFile.closeData();

  nxsFile.openData("SPEC");
  NXIntArray specTable(nxsFile.getData<int32_t>());
  nxsFile.closeData();

  nxsFile.openData("UDET");
  NXIntArray udetTable(nxsFile.getData<int32_t>());
  nxsFile.closeData();

  createGroupingsFromTables(specTable->data(), udetTable->data(), (*ndets)[0]);
}

/**
 * Create the grouping map from the tables
 * @param specTable :: An array of spectrum numbers
 * @param udetTable :: An array of detector IDs
 * @param ndets :: The size of the two arrays
 */
void CreateSimulationWorkspace::createGroupingsFromTables(int *specTable,
                                                          int *udetTable,
                                                          int ndets) {
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
      m_detGroups.insert(std::make_pair(specNo, group));
    }
  }
}

/**
 * @returns The bin bounadries for the new workspace
 */
MantidVecPtr CreateSimulationWorkspace::createBinBoundaries() const {
  const std::vector<double> rbparams = getProperty("BinParams");
  MantidVecPtr binsPtr;
  MantidVec &newBins = binsPtr.access();
  const int numBoundaries =
      Mantid::Kernel::VectorHelper::createAxisFromRebinParams(rbparams,
                                                              newBins);
  if (numBoundaries <= 2) {
    throw std::invalid_argument(
        "Error in BinParams - Gave invalid number of bin boundaries: " +
        boost::lexical_cast<std::string>(numBoundaries));
  }
  return binsPtr;
}

/**
 * Apply the created mapping to the workspace
 */
void CreateSimulationWorkspace::applyDetectorMapping() {
  size_t wsIndex(0);
  for (auto iter = m_detGroups.begin(); iter != m_detGroups.end(); ++iter) {
    ISpectrum *spectrum = m_outputWS->getSpectrum(wsIndex);
    spectrum->setSpectrumNo(
        static_cast<specid_t>(wsIndex + 1)); // Ensure a contiguous mapping
    spectrum->clearDetectorIDs();
    spectrum->addDetectorIDs(iter->second);
    ++wsIndex;
  }
}

/**
 * Apply any instrument adjustments from the file
 * @param filename :: The file to take the positions
 */
void CreateSimulationWorkspace::adjustInstrument(const std::string &filename) {
  // If requested update the instrument to positions in the raw file
  const Geometry::ParameterMap &pmap = m_outputWS->instrumentParameters();
  Geometry::Instrument_const_sptr instrument = m_outputWS->getInstrument();
  boost::shared_ptr<Geometry::Parameter> updateDets =
      pmap.get(instrument->getComponentID(), "det-pos-source");
  if (!updateDets)
    return; // No tag, use IDF

  std::string value = updateDets->value<std::string>();
  if (value.substr(0, 8) == "datafile") {
    IAlgorithm_sptr updateInst =
        createChildAlgorithm("UpdateInstrumentFromFile", 0.75, 1.0);
    updateInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_outputWS);
    updateInst->setPropertyValue("Filename", filename);
    if (value == "datafile-ignore-phi") {
      updateInst->setProperty("IgnorePhi", true);
      g_log.information("Detector positions in IDF updated with positions in "
                        "the data file except for the phi values");
    } else {
      g_log.information(
          "Detector positions in IDF updated with positions in the data file");
    }
    // We want this to throw if it fails to warn the user that the information
    // is not correct.
    updateInst->execute();
  }
}

} // namespace DataHandling
} // namespace Mantid
