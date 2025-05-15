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
#include "MantidNexus/NeXusFile.hpp"
#include <H5Cpp.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::NeXus;

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
  run.addLogData(property.release());
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

bool checkPolarization(const H5::Group &entry) {
  const auto pIn = entry.nameExists(sasDataPin);
  const auto pOut = entry.nameExists(sasDataPout);
  if (pIn != pOut) {
    throw std::invalid_argument("Polarized data requires to have Pin and Pout axes");
  }
  return pIn && pOut;
}

std::pair<std::vector<int>, std::vector<int>> initializeSpinVectors(const H5::Group &group) {
  // Check for polarization first
  std::vector<int> pIn;
  std::vector<int> pOut;
  if (checkPolarization(group)) {
    pIn = H5Util::readArray1DCoerce<int>(group, sasDataPin);
    pOut = H5Util::readArray1DCoerce<int>(group, sasDataPout);
  }
  return std::make_pair(pIn, pOut);
}

std::vector<SpinState> prepareSpinIndexes(const std::vector<int> &pIn, const std::vector<int> &pOut) {
  // Helds spin pair with vector indexes to iterate hyperslab
  auto spinToString = [](const int spinIndex) {
    return spinIndex == 1 ? '+' + std::to_string(spinIndex) : std::to_string(spinIndex);
  };

  std::vector<SpinState> spinIndexes;
  for (size_t i = 0; i < pIn.size(); i++) {
    for (size_t j = 0; j < pOut.size(); j++) {
      SpinState state;
      state.strSpinState = spinToString(pIn.at(i)) + spinToString(pOut.at(j));
      state.indexPin = i;
      state.indexPout = j;
      spinIndexes.push_back(state);
    }
  }
  return spinIndexes;
}

void updateSpinOffset(const size_t indexPin, const size_t indexPout, std::vector<hsize_t> &spinOffset) {
  if (!spinOffset.empty()) {
    spinOffset[0] = indexPin;
    spinOffset[1] = indexPout;
  }
}

void loadPolarizedLogs(const H5::Group &group, const MatrixWorkspace_sptr &workspace) {
  const auto logNames = std::vector({sasSampleEMFieldDirectionAzimuthal, sasSampleEMFieldDirectionPolar,
                                     sasSampleEMFieldDirectionRotation, sasSampleMagneticField});
  for (auto const &log : logNames) {
    auto logValue = getNumDataSetIfExists(group, log);
    if (!logValue.empty()) {
      std::string logUnits;
      H5Util::readStringAttribute(group.openDataSet(log), sasUnitAttr, logUnits);
      addLogToWs(workspace, log, logValue.front(), logUnits);
    }
  }
}

//----- Sample ----- //
std::optional<H5::Group> getGroupIfExists(const std::optional<H5::Group> &group, const std::string &groupName) {
  if (!group.has_value()) {
    return std::nullopt;
  }
  if (const auto entry = group.value(); entry.nameExists(groupName)) {
    return entry.openGroup(groupName);
  }
  return std::nullopt;
}

std::optional<Sample> loadSample(const H5::Group &entry) {
  //  Load Height, Width, and Geometry from the aperture group and save to Sample object.
  const auto &instrumentGroup = getGroupIfExists(entry, sasInstrumentGroupName);
  const auto &apertureGroup = getGroupIfExists(instrumentGroup, sasInstrumentApertureGroupName);
  const auto &sampleGroup = getGroupIfExists(entry, sasInstrumentSampleGroupAttr);

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
    auto &&geometry = getStrDataSetIfExists(apertureGroup.value(), sasInstrumentApertureShape);
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

void loadInstrument(const Mantid::API::MatrixWorkspace_sptr &workspace, const InstrumentNameInfo &instrumentInfo) {
  // Try to load the instrument. If it fails we will continue nevertheless.
  try {
    // Get IDF
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
std::string getStrAttribute(const H5::DataSet &dataSet, const std::string &attrName) {
  std::string attrValue;
  H5Util::readStringAttribute(dataSet, attrName, attrValue);
  return attrValue;
}

enum WorkspaceDataAxes : std::uint8_t { Y = 0, YErr = 1, X = 2, XErr = 3 };
struct WorkspaceDataInserter {
  explicit WorkspaceDataInserter(const MatrixWorkspace_sptr &workspace) : workspace(workspace), axisType(0) {}

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
    if (axisType == Y || axisType == YErr) {
      workspace->setYUnit(getStrAttribute(dataSet, sasUnitAttr));
    } else if (axisType == X || axisType == XErr) {
      workspace->getAxis(0)->setUnit("MomentumTransfer");
    }
  }
  void setAxisType(const int type) { axisType = type; }
  MatrixWorkspace_sptr workspace;
  int axisType;
};

void readDataIntoWorkspace(const H5::DataSet &dataSet, const WorkspaceDataInserter &inserter,
                           const std::vector<hsize_t> &slabShape, const hsize_t nPoints, const hsize_t nHistograms = 1,
                           const std::vector<hsize_t> &spinOffset = {}) {

  // Memory Data Space
  const std::array<hsize_t, 1> memSpaceDimension = {nPoints};
  const H5::DataSpace memSpace(1, memSpaceDimension.data());

  auto position = (nHistograms > 1) ? std::vector<hsize_t>(2, 0) : std::vector<hsize_t>(1, 0);
  size_t histogramIndex = 0;
  if (!spinOffset.empty()) {
    position.insert(position.cbegin(), spinOffset.cbegin(), spinOffset.cend());
    histogramIndex = spinOffset.size();
  }

  const auto fileSpace = dataSet.getSpace();
  auto data = std::vector<double>(nPoints, 0);
  for (size_t index = 0; index < nHistograms; ++index) {
    // Set the dataSpace to a 1D HyperSlab
    fileSpace.selectHyperslab(H5S_SELECT_SET, slabShape.data(), position.data());
    dataSet.read(data.data(), H5Util::getType<double>(), memSpace, fileSpace);
    inserter.insertData(index, data);
    position[histogramIndex]++;
  }
}

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

bool findDefinition(NeXus::File &file) {
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

//----- Transmission ----- //
bool fileHasTransmissionEntry(const H5::Group &entry, const std::string &name) {
  const bool hasTransmission = entry.nameExists(sasTransmissionSpectrumGroupName + "_" + name);
  if (!hasTransmission) {
    g_log.information("NXcanSAS file does not contain transmission for " + name);
  }
  return hasTransmission;
}

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
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadNXcanSAS)

/// constructor
LoadNXcanSAS::LoadNXcanSAS() = default;

int LoadNXcanSAS::confidence(Nexus::NexusDescriptor &descriptor) const {
  const std::string &extn = descriptor.extension();
  if (extn != ".nxs" && extn != ".h5") {
    return 0;
  }
  int confidence = 0;
  ::NeXus::File file(descriptor.filename());
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
  H5::H5File file(fileName, H5F_ACC_RDONLY, NeXus::H5Util::defaultFileAcc());

  const auto entry = file.openGroup(getNameOfEntry(file));
  const auto dataGroup = entry.openGroup(sasDataGroupName);
  const auto dataInfo = getDataSpaceInfo(dataGroup.openDataSet(sasDataI));

  // Setup progress bar
  const auto numberOfSteps = isLoadTransmissionChecked ? dataInfo.spinStates * 5 + 1 : dataInfo.spinStates * 5;
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

Mantid::API::WorkspaceGroup_sptr LoadNXcanSAS::transferFileDataIntoWorkspace(const H5::Group &entry,
                                                                             const DataSpaceInformation &dataInfo,
                                                                             const InstrumentNameInfo &instrumentInfo) {
  const auto dataGroup = entry.openGroup(sasDataGroupName);
  const auto states = prepareDataDimensions(dataGroup, dataInfo);
  auto spinOffset = dataInfo.spinStates > 1 ? std::vector<hsize_t>(2, 0) : std::vector<hsize_t>();

  auto dataOut = std::make_shared<WorkspaceGroup>();
  const auto sample = loadSample(entry); // Sample should be similar in all workspaces
  // spinStr will be empty if data is not polarized -> Only one output workspace
  for (const auto &state : states) {
    const auto &[spinStr, indexPin, indexPout] = state;
    auto ws = createWorkspace(dataInfo);
    updateSpinOffset(indexPin, indexPout, spinOffset);

    loadMetadata(entry, ws, instrumentInfo, sample, !spinStr.empty());
    loadData(dataGroup, ws, spinOffset);

    if (!spinStr.empty()) {
      addLogToWs<std::string>(ws, NX_SPIN_LOG, spinStr);
    }
    dataOut->addWorkspace(ws);
  }
  return dataOut;
}
std::vector<SpinState> LoadNXcanSAS::prepareDataDimensions(const H5::Group &group,
                                                           const DataSpaceInformation &dataInfo) {
  const auto &[pIn, pOut] = initializeSpinVectors(group);
  const auto spinPairs =
      !pIn.empty() && !pOut.empty() ? std::optional(std::make_pair(pIn.size(), pOut.size())) : std::nullopt;

  const auto spinStates = spinPairs.has_value() ? prepareSpinIndexes(pIn, pOut) :
                                                /* default unpolarized: 1 state */ std::vector({SpinState()});
  // prepare data dimensions and offset
  m_dataDims = std::make_unique<DataDimensions>(dataInfo.dimBin, dataInfo.dimSpectrumAxis, spinPairs);
  return spinStates;
}

void LoadNXcanSAS::loadMetadata(const H5::Group &entry, const MatrixWorkspace_sptr &workspace,
                                const InstrumentNameInfo &instrumentInfo, const std::optional<Sample> &sample,
                                const bool hasPolarizedData) const {
  // Load logs
  m_progress->report("Loading logs.");
  loadLogs(entry, workspace);
  if (hasPolarizedData && entry.nameExists(sasInstrumentSampleGroupAttr)) {
    loadPolarizedLogs(entry.openGroup(sasInstrumentSampleGroupAttr), workspace);
  }

  // Load title
  m_progress->report("Loading title");
  loadTitle(entry, workspace);

  // Load sample info
  m_progress->report("Loading sample.");
  if (sample.has_value()) {
    workspace->mutableSample() = sample.value();
  }
  // Load instrument
  m_progress->report("Loading instrument.");
  loadInstrument(workspace, instrumentInfo);
}

void LoadNXcanSAS::loadData(const H5::Group &dataGroup, const Mantid::API::MatrixWorkspace_sptr &workspace,
                            const std::vector<hsize_t> &spinOffset) const {
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

  for (auto const &[setName, axesIndex] : dataSets) {
    auto dataSet = dataGroup.openDataSet(setName);
    dataInserter.setAxisType(axesIndex);

    const bool isXAxis = axesIndex == WorkspaceDataAxes::X || axesIndex == WorkspaceDataAxes::XErr;
    if (isXAxis && slabShape.size() > 2) {
      // cut slabShape for Q data
      slabShape = std::vector<hsize_t>(slabShape.cbegin() + 2, slabShape.cend());
    }
    readDataIntoWorkspace(dataSet, dataInserter, slabShape, nPoints, nHisto,
                          isXAxis ? std::vector<hsize_t>() : spinOffset);
    dataInserter.setUnits(dataSet);
  }

  // Qy is inserted a bit differently
  if (dataGroup.nameExists(sasDataQy)) {
    readQyInto2DWorkspace(dataGroup.openDataSet(sasDataQy), workspace, m_dataDims->getNumberOfHistograms());
  }
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
