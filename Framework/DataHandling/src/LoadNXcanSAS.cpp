// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNXcanSAS.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidKernel/Logger.h"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NexusFile.h"
#include <H5Cpp.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::Nexus;

namespace {
Mantid::Kernel::Logger g_log("LoadNXcanSAS");
const std::string NX_SPIN_LOG = "spin_state_NXcanSAS";
const std::map<std::string, int> SAMPLE_GEOMETRIES = {
    {"cylinder", 1}, {"flat plate", 2}, {"flatplate", 2}, {"disc", 3}};

std::string getNameOfEntry(const H5::H5File &root) {
  auto numberOfObjects = root.getNumObjs();
  if (numberOfObjects != 1) {
    throw std::invalid_argument("LoadNXcanSAS: Trying to load multiperiod "
                                "data. This is currently not supported.");
  }
  auto objectType = root.getObjTypeByIdx(0);
  if (objectType != H5G_GROUP) {
    throw std::invalid_argument("LoadNXcanSAS: The object below the root is not a H5::Group.");
  }

  return root.getObjnameByIdx(0);
}

/**
 * Tries to find a nexus or sas entry definition on the loaded file to provide a confidence estimation for the loader.
 * @param file: Reference to loaded file
 */
bool findDefinition(Mantid::Nexus::File &file) {
  bool foundDefinition = false;
  const auto entries = file.getEntries();
  for (const auto &[sasEntry, nxEntry] : entries) {
    if (nxEntry == sasEntryClassAttr || nxEntry == nxEntryClassAttr) {
      file.openGroup(sasEntry, nxEntry);
      file.openData(sasEntryDefinition);
      const auto definitionFromFile = file.getStrData();
      if (definitionFromFile == sasEntryDefinitionFormat) {
        foundDefinition = true;
        break;
      }
      file.closeData();
      file.closeGroup();
    }
  }
  return foundDefinition;
}

Mantid::API::MatrixWorkspace_sptr createWorkspace(const DataSpaceInformation &dimInfo, const bool asHistogram = false) {
  const auto &[dimSpectrumAxis, dimBin, _] = dimInfo;
  // Create a workspace based on the dataSpace information
  return Mantid::API::WorkspaceFactory::Instance().create(
      "Workspace2D", dimSpectrumAxis /*NHisto*/, asHistogram ? dimBin + 1 : dimBin /*xdata*/, dimBin /*ydata*/);
}

std::vector<double> getNumDataSetIfExists(const H5::Group &group, const std::string &datasetName) {
  if (group.nameExists(datasetName)) {
    return H5Util::readArray1DCoerce<double>(group, datasetName);
  }
  return {};
}

std::string getStrDataSetIfExists(const H5::Group &group, const std::string &datasetName) {
  if (group.nameExists(datasetName)) {
    return H5Util::readString(group, datasetName);
  }
  return {};
}

//----- Logs ----- //

template <typename T>
void addLogToWs(const MatrixWorkspace_sptr &workspace, const std::string &logName, const T &logValue,
                const std::string &logUnits = "") {
  auto &run = workspace->mutableRun();
  auto property = std::make_unique<PropertyWithValue<T>>(logName, logValue);
  if (!logUnits.empty()) {
    property->setUnits(logUnits);
  }
  run.addProperty(std::move(property));
}

void loadLogs(const H5::Group &entry, const MatrixWorkspace_sptr &workspace) {
  const auto addLogFromGroup = [workspace](const H5::Group &group, const std::string &sasTerm,
                                           const std::string &propertyName) {
    const auto propValue = getStrDataSetIfExists(group, sasTerm);
    if (!propValue.empty()) {
      addLogToWs(workspace, propertyName, propValue);
    }
  };
  // Load UserFile, BatchFile and Run if present on file
  const auto process = entry.openGroup(sasProcessGroupName);
  addLogFromGroup(process, sasProcessTermUserFile, sasProcessUserFileInLogs);
  addLogFromGroup(process, sasProcessTermBatchFile, sasProcessBatchFileInLogs);
  addLogFromGroup(entry, sasEntryRun, sasEntryRunInLogs);
}

void loadTitle(const H5::Group &entry, const Workspace_sptr &workspace) {
  if (entry.nameExists(sasEntryTitle)) {
    workspace->setTitle(H5Util::readString(entry, sasEntryTitle));
  }
}

//----- Polarization ----- //

/**
 * Checks wheter there is Pin or Pout axis in the Data Group of the loaded file.
 * @param group: H5Group to inspect
 */
bool checkPolarization(const H5::Group &group) {
  const auto pIn = group.nameExists(sasDataPin);
  const auto pOut = group.nameExists(sasDataPout);
  if (pIn != pOut) {
    throw std::invalid_argument("Polarized data requires to have Pin and Pout axes");
  }
  return pIn;
}

/**
 * Loads Pin and Pout axis from the Data Group onto integer vectors and packs them in a std::pair
 * @param group: H5Group group to inspect
 * @return Pin, Pout vectors packed in a std::pair
 */
std::pair<std::vector<int>, std::vector<int>> loadSpinVectors(const H5::Group &group) {
  // Check for polarization first
  std::vector<int> pIn;
  std::vector<int> pOut;
  if (checkPolarization(group)) {
    pIn = H5Util::readArray1DCoerce<int>(group, sasDataPin);
    pOut = H5Util::readArray1DCoerce<int>(group, sasDataPout);
  }
  return std::make_pair(pIn, pOut);
}

/**
 * Generates, for every value of (Pin, Pout), a struct SpinState that contains
 * a string labeling the corresponding spin states
 * (i.e "+1+1" for Pin(i) = 1  ,Pout(j) = 1)
 * as well as the vector indexes that retrieve that state.
 * Used to offset the signal hyperslab as well as record the spin state on the sample logs
 * @param pIn: Pin axes, a vector of with polarization states {-1,+1,0} .
 * @param pOut: Pout axes, a vector of int with polarization states {-1,+1,0}.
 * @return vector of SpinState structs.
 */
std::vector<SpinState> prepareSpinIndexes(const std::vector<int> &pIn, const std::vector<int> &pOut) {
  // Holds spin pair with vector indexes to iterate hyperslab
  auto spinToString = [](const int spinIndex) {
    return spinIndex == 1 ? '+' + std::to_string(spinIndex) : std::to_string(spinIndex);
  };

  std::vector<SpinState> spinIndexes;
  for (size_t i = 0; i < pIn.size(); i++) {
    for (size_t j = 0; j < pOut.size(); j++) {
      SpinState state;
      state.strSpinState = spinToString(pIn.at(i)) + spinToString(pOut.at(j));
      state.spinIndexPair = std::make_pair(i, j);
      spinIndexes.push_back(std::move(state));
    }
  }
  return spinIndexes;
}

/**
 * Loads logs corresponding to polarization metadata stored in the H5 File.
 * @param group: H5 Group to read data from.
 * @param workspace: Matrix workspace in which to load logs.
 */
void loadPolarizedLogs(const H5::Group &group, const MatrixWorkspace_sptr &workspace) {
  const auto logNames = std::vector({sasSampleEMFieldDirectionAzimuthal, sasSampleEMFieldDirectionPolar,
                                     sasSampleEMFieldDirectionRotation, sasSampleMagneticField});
  for (const auto &log : logNames) {
    auto logValue = getNumDataSetIfExists(group, log);
    if (!logValue.empty()) {
      std::string logUnits;
      H5Util::readStringAttribute(group.openDataSet(log), sasUnitAttr, logUnits);
      addLogToWs(workspace, log, logValue.front(), logUnits);
    }
  }
}

//----- Sample ----- //
std::optional<H5::Group> getGroupIfExists(const H5::Group &group, const std::string &groupName) {
  if (group.nameExists(groupName)) {
    return group.openGroup(groupName);
  }
  return std::nullopt;
}

/**
 * Loads sample information from the H5 file and stores it in a sample object if such information exists.
 * @param group: H5 Group to read data from.
 * @return optional Sample object
 */
std::optional<Sample> loadSample(const H5::Group &group) {
  //  Load Height, Width, and Geometry from the aperture group and save to Sample object.
  const auto &instrumentGroup = getGroupIfExists(group, sasInstrumentGroupName);
  auto apertureGroup = std::optional<H5::Group>();
  if (instrumentGroup.has_value()) {
    apertureGroup = getGroupIfExists(instrumentGroup.value(), sasInstrumentApertureGroupName);
  }
  const auto &sampleGroup = getGroupIfExists(group, sasInstrumentSampleGroupAttr);

  if (!apertureGroup.has_value() && !sampleGroup.has_value()) {
    return std::nullopt;
  }
  auto sample = Sample();
  if (apertureGroup.has_value()) {
    const auto &height = getNumDataSetIfExists(apertureGroup.value(), sasInstrumentApertureGapHeight);
    if (!height.empty()) {
      sample.setHeight(height.front());
    }
    const auto &width = getNumDataSetIfExists(apertureGroup.value(), sasInstrumentApertureGapWidth);
    if (!width.empty()) {
      sample.setWidth(width.front());
    }
    auto geometry = getStrDataSetIfExists(apertureGroup.value(), sasInstrumentApertureShape);
    boost::to_lower(geometry);
    if (!geometry.empty()) {
      SAMPLE_GEOMETRIES.contains(geometry) ? sample.setGeometryFlag(SAMPLE_GEOMETRIES.at(geometry))
                                           : sample.setGeometryFlag(0);
    }
  }

  // Load thickness from the sample group and save to the Sample object.
  if (sampleGroup.has_value()) {
    const auto &thickness = getNumDataSetIfExists(sampleGroup.value(), sasInstrumentSampleThickness);
    if (!thickness.empty()) {
      sample.setThickness(thickness.front());
    }
  }
  return sample;
}

/**
 * Loads an instrument into the workspace
 * @param workspace: Matrix workspace to load instrument to.
 * @param instrumentInfo: Struct containing the instrument name and the idf.
 */
void loadInstrument(const Mantid::API::MatrixWorkspace_sptr &workspace, const InstrumentNameInfo &instrumentInfo) {
  // Try to load the instrument. If it fails we will continue nevertheless.
  try {
    const auto instAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
    instAlg->initialize();
    instAlg->setChild(true);
    instAlg->setProperty("Workspace", workspace);
    instAlg->setProperty("InstrumentName", instrumentInfo.instrumentName);
    if (!instrumentInfo.idf.empty()) {
      instAlg->setProperty("Filename", instrumentInfo.idf);
    }
    instAlg->setProperty("RewriteSpectraMap", "False");
    instAlg->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm.");
  } catch (std::runtime_error &) {
    g_log.information("Unable to successfully run LoadInstrument Child Algorithm.");
  }
}

//----- Data ----- //

/**
 * Prepares the slabShape and offset vectors to extract data from a given slice of the signal or Q datasets in the H5
 * file.
 * The Y and X axis of the workspace are stored in the H5 File as signal (I) and Q datasets. For non-polarized data,
 * signal (I) can either be 1D or 2D (1 x M vector or N x M matrix - N: number of spectra, M:number of bins per
 * spectra-), and Q has the same dimensions as I (vector for 1D contained in a Q dataset, or a matrix for 2D split into
 * two datasets Qx and Qy). When the data is polarized, the signal dataset has a larger number of dimensions to
 * accomodate all the spin states that were saved from different workspaces. For 1D data: 2 x 1 x M (half-polarized) or
 * 2 x 2 x M (full-polarized). For 2D data: 2 x 1 x N x M (half-polarized) or 2 x 2 x N (full-polarized). As the Q
 * ranges were the same for all the spin state workspaces saved into the signal dataset, the dimensions of the Q
 * datasets are the same as in the non-polarized data, therefore, we trim them. (This discussion is equivalent for axes
 * XErr and YErr)
 * @param axesIndex: Index to know which kind of axes the data is going to be stored in the workspace (X, Y, XErr,
 * YErr). If data is X or XErr, the dimensions of the slab and offset vectors are different.
 * @param spinIndexPair: pair of size_t containing the offset indexes on the hyperslab if there is polarized data.
 * @param slabShape: Reference to vector containing the shape of the slab from which to slice the signal or Q datasets
 * on the H5 File.
 * @return Vector with offset position on the hyperslab of the signal or Q datasets.
 */
std::vector<hsize_t> updateOffset(const int axesIndex, const std::pair<size_t, size_t> &spinIndexPair,
                                  std::vector<hsize_t> &slabShape) {
  const bool isXAxis = axesIndex == WorkspaceDataAxes::X || axesIndex == WorkspaceDataAxes::XErr;
  const auto &[indexPin, indexPout] = spinIndexPair;
  auto position = std::vector<hsize_t>(slabShape.size(), 0);
  if (slabShape.size() > 2) {
    if (isXAxis) {
      // cut slabShape for Q data
      slabShape = std::vector<hsize_t>(slabShape.cbegin() + 2, slabShape.cend());
      position = std::vector<hsize_t>(position.cbegin() + 2, position.cend());
    } else {
      position.at(0) = indexPin;
      position.at(1) = indexPout;
    }
  }
  return position;
}

std::string getStrAttribute(const H5::DataSet &dataSet, const std::string &attrName) {
  std::string attrValue;
  H5Util::readStringAttribute(dataSet, attrName, attrValue);
  return attrValue;
}

/**
 * Struct used to store data in different axes on a workspace, as well as set the units. Uses the WorkspaceDataAxes enum
 * to select the workspace axis in which data will be inserted from the file.
 */
struct WorkspaceDataInserter {
  explicit WorkspaceDataInserter(MatrixWorkspace_sptr workspace) : workspace(std::move(workspace)), axisType(0) {}

  void insertData(const size_t index, const std::vector<double> &data) const {
    switch (axisType) {
    case Y:
      workspace->mutableY(index) = data;
      break;
    case YErr:
      workspace->mutableE(index) = data;
      break;
    case X:
      workspace->mutableX(index) = data;
      break;
    case XErr:
      workspace->setPointStandardDeviations(index, data);
      break;
    default:
      throw std::runtime_error("Provided axis is not compatible with workspace.");
    }
  }

  void setUnits(const H5::DataSet &dataSet) const {
    if (axisType == Y) {
      workspace->setYUnit(getStrAttribute(dataSet, sasUnitAttr));
    } else if (axisType == X) {
      workspace->getAxis(0)->setUnit("MomentumTransfer");
    }
  }

  void setAxisType(const int type) { axisType = type; }
  MatrixWorkspace_sptr workspace;
  int axisType;
};

/**
 * Reads data from the h5 file and stores in the workspace through the data inserter struct
 * @param dataSet: Dataset to read data from
 * @param inserter: Inserter struct helping to insert data into the appropriate axes of the workspace
 * @param slabShape: Vector containg the shape of the slice that's to be extracted from the dataset
 * @param nPoints : Number of points (or bins) to select in the 1D slab.
 * @param nHistograms: Number of histograms (iteration reference)
 * @param offset: Offset position on the dataset.
 */
void readDataIntoWorkspace(const H5::DataSet &dataSet, const WorkspaceDataInserter &inserter,
                           const std::vector<hsize_t> &slabShape, const hsize_t nPoints, const hsize_t nHistograms,
                           std::vector<hsize_t> &offset) {

  // Memory Data Space
  const std::array<hsize_t, 1> memSpaceDimension = {nPoints};
  const H5::DataSpace memSpace(1, memSpaceDimension.data());
  const bool isPolarizedDataSet = slabShape.size() > 2;
  const auto histogramIndex = isPolarizedDataSet ? slabShape.size() - 2 : 0;

  const auto fileSpace = dataSet.getSpace();
  auto data = std::vector<double>(nPoints, 0);
  for (size_t index = 0; index < nHistograms; ++index) {
    // Set the dataSpace to a 1D HyperSlab
    fileSpace.selectHyperslab(H5S_SELECT_SET, slabShape.data(), offset.data());
    dataSet.read(data.data(), H5Util::getType<double>(), memSpace, fileSpace);
    inserter.insertData(index, data);
    offset.at(histogramIndex)++;
  }
}

/**
 * Extracts the first column of the Qy dataset from the H5 File and uses it to construct a numeric axis on the
 * workspace.
 * @param dataSet: Dataset to read data from
 * @param workspace: Matrix workspace to set the axis to.
 * @param nHistograms: Number of histograms (iteration reference)
 */
void readQyInto2DWorkspace(const H5::DataSet &dataSet, const Mantid::API::MatrixWorkspace_sptr &workspace,
                           const hsize_t nHistograms) {
  // Size of single slab
  const std::array<hsize_t, 2> slabShape = {nHistograms, 1};
  const std::array<hsize_t, 2> position = {0, 0};

  // Memory Data Space
  const std::array<hsize_t, 1> memSpaceDimensions = {nHistograms};
  const H5::DataSpace memSpace(1, memSpaceDimensions.data());

  // Select the HyperSlab
  const auto fileSpace = dataSet.getSpace();
  fileSpace.selectHyperslab(H5S_SELECT_SET, slabShape.data(), position.data());

  // Read the data
  auto data = std::vector<double>(nHistograms);
  dataSet.read(data.data(), H5Util::getType<double>(), memSpace, fileSpace);

  auto newAxis = std::make_unique<Mantid::API::NumericAxis>(data);
  workspace->replaceAxis(1, std::move(newAxis));

  // Set the axis units
  workspace->getAxis(1)->setUnit("MomentumTransfer");
}

//----- Transmission ----- //
bool fileHasTransmissionEntry(const H5::Group &entry, const std::string &name) {
  const bool hasTransmission = entry.nameExists(sasTransmissionSpectrumGroupName + "_" + name);
  if (!hasTransmission) {
    g_log.information("NXcanSAS file does not contain transmission for " + name);
  }
  return hasTransmission;
}

/**
 * Takes transmission signal and Q from the corresponding group in the H5 File and puts in a tranmission workspace
 * @param transmission: Group containing transmission or transmission can datasets.
 * @param workspace: Matrix workspace containing transmission data.
 */
void loadTransmissionData(const H5::Group &transmission, const Mantid::API::MatrixWorkspace_sptr &workspace) {
  //-----------------------------------------
  // Load T
  workspace->mutableY(0) = H5Util::readArray1DCoerce<double>(transmission, sasTransmissionSpectrumT);
  //-----------------------------------------
  // Load Tdev
  workspace->mutableE(0) = H5Util::readArray1DCoerce<double>(transmission, sasTransmissionSpectrumTdev);
  //-----------------------------------------
  // Load Lambda. A bug in older versions (fixed in 6.0) allowed the
  // transmission lambda points to be saved as bin edges rather than points as
  // required by the NXcanSAS standard. We allow loading those files and convert
  // to points on the fly
  std::vector<double> lambda;
  H5Util::readArray1DCoerce(transmission, sasTransmissionSpectrumLambda, lambda);
  if (lambda.size() == workspace->blocksize())
    workspace->setPoints(0, std::move(lambda));
  else if (lambda.size() == workspace->blocksize() + 1)
    workspace->setBinEdges(0, std::move(lambda));
  else {
    const std::string objectName{transmission.getObjName()};
    throw std::runtime_error("Unexpected array size for lambda in transmission group '" + objectName +
                             "'. Expected length=" + std::to_string(workspace->blocksize()) +
                             ", found length=" + std::to_string(lambda.size()));
  }
  workspace->getAxis(0)->setUnit("Wavelength");
  workspace->setYUnitLabel("Transmission");
  // Set to distribution
  workspace->setDistribution(true);
}

} // namespace

namespace Mantid::DataHandling::NXcanSAS {
// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_LAZY_FILELOADER_ALGORITHM(LoadNXcanSAS)

/// constructor
LoadNXcanSAS::LoadNXcanSAS() = default;

int LoadNXcanSAS::confidence(Nexus::NexusDescriptorLazy &descriptor) const {
  const std::string &extn = descriptor.extension();
  if (extn != ".nxs" && extn != ".h5") {
    return 0;
  }
  int confidence = 0;
  Mantid::Nexus::File file(descriptor.filename());
  // Check if there is an entry root/SASentry/definition->NXcanSAS
  try {
    if (findDefinition(file)) {
      confidence = 95;
    }
  } catch (...) {
  }

  return confidence;
}

void LoadNXcanSAS::init() {
  // Declare required input parameters for algorithm
  const std::vector<std::string> exts{".nxs", ".h5"};
  declareProperty(std::make_unique<Mantid::API::FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the NXcanSAS file to read, as a full or relative path.");
  declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>("OutputWorkspace", "",
                                                                                           Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service. For multiperiod "
                  "files, one workspace may be generated for each period. "
                  "Currently only one workspace can be saved at a time so "
                  "multiperiod Mantid files are not generated.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadTransmission", false, Direction::Input),
                  "Load the transmission related data from the file if it is present "
                  "(optional, default False).");
}

void LoadNXcanSAS::exec() {
  const std::string fileName = getPropertyValue("Filename");
  const bool isLoadTransmissionChecked = getProperty("LoadTransmission");
  H5::H5File file(fileName, H5F_ACC_RDONLY, Nexus::H5Util::defaultFileAcc());

  const auto entry = file.openGroup(getNameOfEntry(file));
  const auto dataGroup = entry.openGroup(sasDataGroupName);
  const auto dataInfo = getDataSpaceInfo(dataGroup.openDataSet(sasDataI));

  // Setup progress bar
  const size_t stepsPerSpinState = dataInfo.spinStates * 5;
  const auto numberOfSteps = isLoadTransmissionChecked ? stepsPerSpinState + 1 : stepsPerSpinState;
  m_progress = std::make_unique<API::Progress>(this, 0.1, 1.0, numberOfSteps);

  // Load metadata and data into output workspace
  const InstrumentNameInfo instrumentInfo(entry);
  const auto wsGroup = transferFileDataIntoWorkspace(entry, dataInfo, instrumentInfo);

  // Load Transmissions
  if (isLoadTransmissionChecked) {
    m_progress->report("Loading transmissions.");
    // Load sample transmission
    loadTransmission(entry, sasTransmissionSpectrumNameSampleAttrValue, instrumentInfo);
    // Load can transmission
    loadTransmission(entry, sasTransmissionSpectrumNameCanAttrValue, instrumentInfo);
  }

  const auto wsOut = dataInfo.spinStates == 1 ? wsGroup->getItem(0) : std::dynamic_pointer_cast<Workspace>(wsGroup);
  file.close();
  setProperty("OutputWorkspace", wsOut);
}

/**
 * Loads metadata from H5 File into a Workspace: sample logs, instrument and sample information.
 * @param group: H5 group
 * @param workspace: Matrix workspace in which to store metadata
 * @param instrumentInfo: Struct containing instrument name and idf.
 * @param sample: Optional sample object. Sample data won't be stored in workspace if this is empty.
 * @param hasPolarizedData: Flag to check if polarized logs have to be loaded.
 */
void LoadNXcanSAS::loadMetadata(const H5::Group &group, const MatrixWorkspace_sptr &workspace,
                                const InstrumentNameInfo &instrumentInfo, const std::optional<Sample> &sample,
                                const bool hasPolarizedData) const {
  // Load logs
  m_progress->report("Loading logs.");
  loadLogs(group, workspace);
  if (hasPolarizedData && group.nameExists(sasInstrumentSampleGroupAttr)) {
    loadPolarizedLogs(group.openGroup(sasInstrumentSampleGroupAttr), workspace);
  }

  // Load title
  m_progress->report("Loading title");
  loadTitle(group, workspace);

  // Load sample info
  m_progress->report("Loading sample.");
  if (sample.has_value()) {
    workspace->mutableSample() = sample.value();
  }
  // Load instrument
  m_progress->report("Loading instrument.");
  loadInstrument(workspace, instrumentInfo);
}

/**
 * Loads signal, Q and error data from the H5 file into the corresponding matrix workspace
 * @param dataGroup: H5 group containing the target datasets
 * @param workspace: Matrix workspace in which to store data
 * @param spinIndexPair: pair containing the spin state to select the appropriate slice on the signal dataset.
 */
void LoadNXcanSAS::loadData(const H5::Group &dataGroup, const Mantid::API::MatrixWorkspace_sptr &workspace,
                            const std::pair<size_t, size_t> &spinIndexPair) const {
  m_progress->report("Loading data.");
  workspace->setDistribution(true);
  WorkspaceDataInserter dataInserter(workspace);
  auto dataSets = std::map<std::string, int>{
      {sasDataI, 0}, {sasDataIdev, 1}, {m_dataDims->getNumberOfHistograms() > 1 ? sasDataQx : sasDataQ, 2}};

  if (dataGroup.nameExists(sasDataQdev)) {
    dataSets.insert({sasDataQdev, 3});
  }
  auto slabShape = m_dataDims->getSlabShape();
  const auto nPoints = m_dataDims->getNumberOfPoints();
  const auto nHisto = m_dataDims->getNumberOfHistograms();

  for (const auto &[setName, axesIndex] : dataSets) {
    auto dataSet = dataGroup.openDataSet(setName);
    auto offset = updateOffset(axesIndex, spinIndexPair, slabShape);
    dataInserter.setAxisType(axesIndex);
    readDataIntoWorkspace(dataSet, dataInserter, slabShape, nPoints, nHisto, offset);
    dataInserter.setUnits(dataSet);
  }

  // Qy is inserted a bit differently
  if (dataGroup.nameExists(sasDataQy)) {
    readQyInto2DWorkspace(dataGroup.openDataSet(sasDataQy), workspace, m_dataDims->getNumberOfHistograms());
  }
}

/**
 * Prepares the DataDimensions struct that contains the shapes of the dataset objects, as well as the
 * information about the spin vectors stored in the file.
 * @param group: H5 group containing the target datasets
 * @param dataInfo: struct containing information about the dimensions of the signal dataset
 */
std::vector<SpinState> LoadNXcanSAS::prepareDataDimensions(const H5::Group &group,
                                                           const DataSpaceInformation &dataInfo) {
  const auto &[pIn, pOut] = loadSpinVectors(group);
  const auto spinPairs =
      !pIn.empty() && !pOut.empty() ? std::optional(std::make_pair(pIn.size(), pOut.size())) : std::nullopt;

  const auto spinStates = spinPairs.has_value() ? prepareSpinIndexes(pIn, pOut) :
                                                /* default unpolarized: 1 state */ std::vector({SpinState()});
  // prepare data dimensions and offset
  m_dataDims = std::make_unique<DataDimensions>(dataInfo.dimBin, dataInfo.dimSpectrumAxis, spinPairs);
  return spinStates;
}

/**
 * Prepares the DataDimensions struct that contains the shapes of the dataset objects, as well as the
 * information about the spin vectors stored in the file.
 * @param group: H5 group containing the target datasets
 * @param dataInfo: Struct containing information about the dimensions of the signal dataset
 * @param instrumentInfo: Struct containing information about instrument name and idf
 */
Mantid::API::WorkspaceGroup_sptr LoadNXcanSAS::transferFileDataIntoWorkspace(const H5::Group &group,
                                                                             const DataSpaceInformation &dataInfo,
                                                                             const InstrumentNameInfo &instrumentInfo) {
  const auto dataGroup = group.openGroup(sasDataGroupName);
  const auto states = prepareDataDimensions(dataGroup, dataInfo);
  const auto wsName = getPropertyValue("OutputWorkspace");

  auto dataOut = std::make_shared<WorkspaceGroup>();
  const auto sample = loadSample(group); // Sample should be similar in all workspaces
  // spinStr will be empty if data is not polarized -> Only one output workspace
  for (const auto &[spinStr, spinIndexPair] : states) {
    auto ws = createWorkspace(dataInfo);

    loadMetadata(group, ws, instrumentInfo, sample, !spinStr.empty());
    loadData(dataGroup, ws, spinIndexPair);

    if (!spinStr.empty()) {
      addLogToWs(ws, NX_SPIN_LOG, spinStr);
      ws->setTitle(wsName + "_" + spinStr);
    }
    dataOut->addWorkspace(ws);
  }
  return dataOut;
}

void LoadNXcanSAS::loadTransmission(const H5::Group &entry, const std::string &name,
                                    const InstrumentNameInfo &instrumentInfo) {
  if (!fileHasTransmissionEntry(entry, name)) {
    return;
  }
  const auto transmission = entry.openGroup(sasTransmissionSpectrumGroupName + "_" + name);
  const auto tDataSet = transmission.openDataSet(sasTransmissionSpectrumT);
  // Create a 1D workspace
  const auto workspace = createWorkspace(getDataSpaceInfo(tDataSet), true);
  // Load logs
  loadLogs(entry, workspace);
  loadTitle(entry, workspace);
  workspace->setTitle(workspace->getTitle() + "_trans_" + name);
  // Load Instrument
  loadInstrument(workspace, instrumentInfo);
  // Load transmission data
  loadTransmissionData(transmission, workspace);
  // Set the workspace on the output
  const std::string propertyName = (name == "sample") ? "TransmissionWorkspace" : "TransmissionCanWorkspace";
  declareProperty(std::make_unique<WorkspaceProperty<>>(propertyName, workspace->getTitle(), Direction::Output),
                  "The transmission workspace");
  setProperty(propertyName, workspace);
}

} // namespace Mantid::DataHandling::NXcanSAS
