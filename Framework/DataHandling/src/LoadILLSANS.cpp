// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLSANS.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/Path.h>

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLSANS)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLSANS::LoadILLSANS()
    : m_supportedInstruments{"D11", "D22", "D33", "D16"}, m_defaultBinning{0, 0}, m_resMode("nominal"), m_isTOF(false),
      m_sourcePos(0.), m_numberOfMonitors(2) {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLSANS::name() const { return "LoadILLSANS"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLSANS::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLSANS::category() const { return "DataHandling\\Nexus;ILL\\SANS"; }

/// Algorithm's summary. @see Algorithm::summery
const std::string LoadILLSANS::summary() const {
  return "Loads ILL nexus files for SANS instruments D11, D16, D22, D33.";
}

//----------------------------------------------------------------------------------------------

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLSANS::confidence(Kernel::NexusDescriptor &descriptor) const {
  // fields existent only at the ILL for SANS machines
  if (descriptor.pathExists("/entry0/mode") &&
      ((descriptor.pathExists("/entry0/reactor_power") && descriptor.pathExists("/entry0/instrument_name")) ||
       (descriptor.pathExists("/entry0/instrument/name") && descriptor.pathExists("/entry0/acquisition_mode") &&
        !descriptor.pathExists("/entry0/instrument/Detector")))) // serves to remove the TOF instruments
  {
    return 80;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLSANS::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "Name of the nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty("Wavelength", 0.0, mustBePositive,
                  "The wavelength of the experiment, in angstroms. Used only for D16. Will "
                  "override the nexus' value if there is one.");
  declareProperty(std::make_unique<FileProperty>("SensitivityMap", "", FileProperty::OptionalLoad, ".nxs"),
                  "Name of the file containing sensitivity map.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLSANS::exec() {
  const std::string filename = getPropertyValue("Filename");
  m_isD16Omega = false;
  getMonitorIndices(filename);
  NXRoot root(filename);
  NXEntry firstEntry = root.openFirstEntry();
  const std::string instrumentPath = LoadHelper::findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);
  Progress progress(this, 0.0, 1.0, 4);
  progress.report("Initializing the workspace for " + m_instrumentName);
  if (m_instrumentName == "D33") {
    initWorkSpaceD33(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();
    const DetectorPosition detPos = getDetectorPositionD33(firstEntry, instrumentPath);
    progress.report("Moving detectors");
    moveDetectorsD33(detPos);
    if (m_isTOF) {
      adjustTOF();
      moveSource();
    }

  } else if (m_instrumentName == "D16") {
    initWorkSpace(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();

    double distance;
    try {
      distance = firstEntry.getFloat(instrumentPath + "/Det/value") / 1000; // mm to metre
    } catch (std::runtime_error &) {
      distance = 0;
    }
    const double angle = firstEntry.getFloat(instrumentPath + "/Gamma/value");
    placeD16(-angle, distance, "detector");

  } else if (m_instrumentName == "D11B") {
    initWorkSpaceD11B(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();

    // we move the parent "detector" component, but since it is at (0,0,0), we
    // need to find the distance it has to move and move it to this position
    double finalDistance = firstEntry.getFloat(instrumentPath + "/Detector 1/det_calc");
    V3D pos = LoadHelper::getComponentPosition(m_localWorkspace, "detector_center");
    double currentDistance = pos.Z();

    moveDetectorDistance(finalDistance - currentDistance, "detector");
    API::Run &runDetails = m_localWorkspace->mutableRun();
    runDetails.addProperty<double>("L2", finalDistance, true);

  } else if (m_instrumentName == "D22B") {
    initWorkSpaceD22B(firstEntry, instrumentPath);

    const std::string backIndex = m_localWorkspace->getInstrument()->getStringParameter("back_detector_index")[0];
    const std::string frontIndex = m_localWorkspace->getInstrument()->getStringParameter("front_detector_index")[0];

    // first we move the central (back) detector
    double distance = firstEntry.getFloat(instrumentPath + "/Detector " + backIndex + "/det" + backIndex + "_calc");
    moveDetectorDistance(distance, "detector_back");
    API::Run &runDetails = m_localWorkspace->mutableRun();
    runDetails.addProperty<double>("L2", distance, true);

    double offset = firstEntry.getFloat(instrumentPath + "/Detector " + backIndex + "/dtr" + backIndex + "_actual");
    moveDetectorHorizontal(-offset / 1000, "detector_back"); // mm to meter

    // then the front (right) one
    distance = firstEntry.getFloat(instrumentPath + "/Detector " + frontIndex + "/det" + frontIndex + "_calc");
    moveDetectorDistance(distance, "detector_front");

    // mm to meter
    offset = firstEntry.getFloat(instrumentPath + "/Detector " + frontIndex + "/dtr" + frontIndex + "_actual");
    moveDetectorHorizontal(-offset / 1000, "detector_front"); // mm to meter
    double angle = firstEntry.getFloat(instrumentPath + "/Detector " + frontIndex + "/dan" + frontIndex + "_actual");
    rotateInstrument(-angle, "detector_front");

  } else {
    // D11 and D22
    initWorkSpace(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();
    double distance = LoadHelper::getDoubleFromNexusPath(firstEntry, instrumentPath + "/detector/det_calc");
    progress.report("Moving detectors");
    moveDetectorDistance(distance, "detector");
    API::Run &runDetails = m_localWorkspace->mutableRun();
    runDetails.addProperty<double>("L2", distance, true);
    if (m_instrumentName == "D22") {
      double offset = LoadHelper::getDoubleFromNexusPath(firstEntry, instrumentPath + "/detector/dtr_actual");
      moveDetectorHorizontal(-offset / 1000, "detector"); // mm to meter
    }
  }
  if (m_instrumentName.find("D16") != std::string::npos && !isDefault("SensitivityMap")) {
    applySensitivityMap();
  }
  progress.report("Setting sample logs");
  setFinalProperties(filename);
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Set member variable with the instrument name
 * @param firstEntry: already opened first entry in nexus
 * @param instrumentNamePath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS::setInstrumentName(const NeXus::NXEntry &firstEntry, const std::string &instrumentNamePath) {
  if (instrumentNamePath.empty()) {
    throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
  }
  m_instrumentName = LoadHelper::getStringFromNexusPath(firstEntry, instrumentNamePath + "/name");
  const auto inst = std::find(m_supportedInstruments.begin(), m_supportedInstruments.end(), m_instrumentName);

  // set alternative version name. Note that D16B is set later, because we need to open the data to distinguish with D16
  if ((m_instrumentName == "D11" || m_instrumentName == "D22") && firstEntry.containsGroup("data1")) {
    m_instrumentName += "B";
  }

  if (inst == m_supportedInstruments.end()) {
    throw std::runtime_error("Instrument " + m_instrumentName +
                             " is not supported. Only D11, D16, D22 and D33 are supported");
  }
  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
}

/**
 * Applies sensitivity map correction to the loaded D16B data.
 *
 */
void LoadILLSANS::applySensitivityMap() {
  // loading nexus processed returns Workspace type, so cannot be used as a child algorithm
  LoadNexusProcessed loader;
  loader.initialize();
  loader.setPropertyValue("Filename", getPropertyValue("SensitivityMap"));
  loader.setPropertyValue("OutputWorkspace", "sensitivity_map");
  loader.execute();
  auto divide = createChildAlgorithm("Divide");
  divide->setProperty("LHSWorkspace", m_localWorkspace);
  divide->setPropertyValue("RHSWorkspace", "sensitivity_map");
  divide->setProperty("AllowDifferentNumberSpectra",
                      true); // in case the localWorkspace contains monitors but sensitivityMap does not
  divide->executeAsChildAlg();
  m_localWorkspace = divide->getProperty("OutputWorkspace");
}

/**
 * Get detector panel distances from the nexus file
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentNamePath : the path inside nexus where the instrument name is written
 * @return a structure with the positions
 */
LoadILLSANS::DetectorPosition LoadILLSANS::getDetectorPositionD33(const NeXus::NXEntry &firstEntry,
                                                                  const std::string &instrumentNamePath) {
  std::string detectorPath(instrumentNamePath + "/detector");
  DetectorPosition pos;
  pos.distanceSampleRear = LoadHelper::getDoubleFromNexusPath(firstEntry, detectorPath + "/det2_calc");
  pos.distanceSampleBottomTop = LoadHelper::getDoubleFromNexusPath(firstEntry, detectorPath + "/det1_calc");
  pos.distanceSampleRightLeft = pos.distanceSampleBottomTop +
                                LoadHelper::getDoubleFromNexusPath(firstEntry, detectorPath + "/det1_panel_separation");
  pos.shiftLeft = LoadHelper::getDoubleFromNexusPath(firstEntry, detectorPath + "/OxL_actual") * 1e-3;
  pos.shiftRight = LoadHelper::getDoubleFromNexusPath(firstEntry, detectorPath + "/OxR_actual") * 1e-3;
  pos.shiftUp = LoadHelper::getDoubleFromNexusPath(firstEntry, detectorPath + "/OyT_actual") * 1e-3;
  pos.shiftDown = LoadHelper::getDoubleFromNexusPath(firstEntry, detectorPath + "/OyB_actual") * 1e-3;
  pos >> g_log.debug();
  return pos;
}

/**
 * @brief LoadILLSANS::getDataDimensions
 * Retrieves physical dimensions of the data from the dataset dimensions.
 * @param data the data set to take the dimensions from
 * @param numberOfChannels
 * @param numberOfTubes
 * @param numberOfPixelsPerTube
 */
void LoadILLSANS::getDataDimensions(const NeXus::NXInt &data, int &numberOfChannels, int &numberOfTubes,
                                    int &numberOfPixelsPerTube) {
  if (m_isD16Omega) {
    numberOfChannels = data.dim0();
    numberOfTubes = data.dim1();
    numberOfPixelsPerTube = data.dim2();
  } else {
    numberOfPixelsPerTube = data.dim1();
    numberOfChannels = data.dim2();
    numberOfTubes = data.dim0();
  }
  g_log.debug() << "Dimensions found:\n- Number of tubes: " << numberOfTubes
                << "\n- Number of pixels per tube: " << numberOfPixelsPerTube
                << "\n- Number of channels: " << numberOfChannels << "\n";
}

/** Gets monitor indices from the scanned variables names and sets them in a vector of indices to be used later
 *
 * @param filename Name of the NeXus file
 */
void LoadILLSANS::getMonitorIndices(const std::string &filename) {
  /*
   * The below tries to access names of scanned variables. These should exist only for omega scans of D16B,
   * and will not be there for other instruments and modes of measurement (D16B gamma scan for example).
   * Therefore, any issue from accessing this data is wrapped in try-catch clause. Assumed is proper order
   * of monitors: Monitor1 should preceed Monitor2 in the scanned variables. The order of these indices
   * in the variables_names will be used to load the monitor data.
   *
   * This method needs to be run before NeXus file is opened with NX library, otherwise it is not possible
   * to open the same file with these two libraries (H5 and NX) simultaneously.
   */
  try {
    H5::H5File h5file(filename, H5F_ACC_RDONLY);
    H5::DataSet scanVarNames = h5file.openDataSet("entry0/data_scan/scanned_variables/variables_names/name");
    H5::DataSpace scanVarNamesSpace = scanVarNames.getSpace();
    const auto nDims = scanVarNamesSpace.getSimpleExtentNdims();
    auto dimsSize = std::vector<hsize_t>(nDims);
    scanVarNamesSpace.getSimpleExtentDims(dimsSize.data(), nullptr);
    std::vector<char *> rdata(dimsSize[0]);
    scanVarNames.read(rdata.data(), scanVarNames.getDataType());
    size_t monitorIndex = 0;
    while (monitorIndex < rdata.size()) {
      const auto varName = std::string(rdata[monitorIndex]);
      if (varName.find("Monitor") != std::string::npos)
        m_monitorIndices.push_back(monitorIndex);
      monitorIndex++;
    }
    h5file.close();
  } catch (...) { // silence all issues accessing the data
    return;
  }
}

/**
 * Loads data for D11, D16 and D22
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS::initWorkSpace(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {
  g_log.debug("Fetching data...");
  std::string path;
  if (firstEntry.containsGroup("data")) {
    path = "data";
  } else {
    path = "data_scan/detector_data/data";
  }
  auto data = LoadHelper::getIntDataset(firstEntry, path);
  data.load();

  // determine if the data comes from a D16 scan
  m_isD16Omega = (data.dim0() >= 1 && data.dim2() > 1 && m_instrumentName == "D16");

  if (m_instrumentName == "D16") {
    const size_t numberOfWiresInD16B = 1152;
    const size_t numberOfPixelsPerWireInD16B = 192;

    // Check if the instrument loaded is D16B. It cannot be differentiated from D16 by the structure of the nexus file
    // only, so we need to check the data size.
    if (data.dim1() == numberOfWiresInD16B && data.dim2() == numberOfPixelsPerWireInD16B) {
      m_instrumentName = "D16B";
      m_isD16Omega = true;
    }
  }

  int numberOfTubes, numberOfPixelsPerTubes, numberOfChannels;
  getDataDimensions(data, numberOfChannels, numberOfTubes, numberOfPixelsPerTubes);

  // For these monochromatic instruments, one bin is "TOF" mode, and more than that is a scan
  MultichannelType type = (numberOfChannels == 1) ? MultichannelType::TOF : MultichannelType::SCAN;

  size_t numberOfHistograms = numberOfPixelsPerTubes * numberOfTubes + m_numberOfMonitors;

  createEmptyWorkspace(numberOfHistograms, numberOfChannels, type);
  loadMetaData(firstEntry, instrumentPath);

  std::vector<double> binning = m_isD16Omega && numberOfChannels > 1
                                    ? getOmegaBinning(firstEntry, "data_scan/scanned_variables/data")
                                    : m_defaultBinning;

  size_t nextIndex;
  nextIndex = loadDataFromTubes(data, binning, 0);
  if (m_instrumentName == "D16B" ||
      (m_isD16Omega && data.dim0() > 1)) // second condition excludes legacy D16 omega scans with single scan point
    loadDataFromD16ScanMonitors(firstEntry, nextIndex, binning);
  else
    loadDataFromMonitors(firstEntry, nextIndex);

  if (data.dim1() == 128) {
    m_resMode = "low";
  }
}

/**
 * @brief LoadILLSANS::initWorkSpaceD11B Load D11B data
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS::initWorkSpaceD11B(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {
  g_log.debug("Fetching data...");

  auto dataCenter = LoadHelper::getIntDataset(firstEntry, "D11/Detector 1/data");
  dataCenter.load();
  auto dataLeft = LoadHelper::getIntDataset(firstEntry, "D11/Detector 2/data");
  dataLeft.load();
  auto dataRight = LoadHelper::getIntDataset(firstEntry, "D11/Detector 3/data");
  dataRight.load();

  size_t numberOfHistograms =
      static_cast<size_t>(dataCenter.dim0() * dataCenter.dim1() + dataRight.dim0() * dataRight.dim1() +
                          dataLeft.dim0() * dataLeft.dim1()) +
      m_numberOfMonitors;

  int numberOfChannels, numberOfPixelsPerTubeCenter, numberOfTubesCenter;
  getDataDimensions(dataCenter, numberOfChannels, numberOfTubesCenter, numberOfPixelsPerTubeCenter);

  MultichannelType type = (numberOfChannels != 1) ? MultichannelType::KINETIC : MultichannelType::TOF;
  createEmptyWorkspace(numberOfHistograms, numberOfChannels, type);
  loadMetaData(firstEntry, instrumentPath);

  // we need to adjust the default binning after loadmetadata
  if (numberOfChannels != 1) {
    std::vector<double> frames(numberOfChannels, 0);
    for (int i = 0; i < numberOfChannels; ++i) {
      frames[i] = i;
    }
    m_defaultBinning.resize(numberOfChannels);
    std::copy(frames.cbegin(), frames.cend(), m_defaultBinning.begin());
  }

  size_t nextIndex;
  nextIndex = loadDataFromTubes(dataCenter, m_defaultBinning, 0, type);
  nextIndex = loadDataFromTubes(dataLeft, m_defaultBinning, nextIndex, type);
  nextIndex = loadDataFromTubes(dataRight, m_defaultBinning, nextIndex, type);
  nextIndex = loadDataFromMonitors(firstEntry, nextIndex, type);

  if (numberOfChannels != 1) {
    // there are a few runs with no 2nd monitor in kinetic, so we load the first monitor once again to preserve the
    // dimensions and x-axis
    if (nextIndex < numberOfHistograms)
      nextIndex = loadDataFromMonitors(firstEntry, nextIndex, type);

    // hijack the second monitor spectrum to store per-frame durations to enable time normalisation
    NXFloat durations = firstEntry.openNXFloat("slices");
    durations.load();
    const HistogramData::Counts histoCounts(durations(), durations() + numberOfChannels);
    m_localWorkspace->setCounts(nextIndex - 1, histoCounts);
    m_localWorkspace->setCountVariances(nextIndex - 1,
                                        HistogramData::CountVariances(std::vector<double>(numberOfChannels, 0)));
  }
}

/**
 * @brief LoadILLSANS::initWorkSpaceD22B Load D22B data
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS::initWorkSpaceD22B(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {
  g_log.debug("Fetching data...");

  auto data1_data = LoadHelper::getIntDataset(firstEntry, "data1");
  data1_data.load();
  auto data2_data = LoadHelper::getIntDataset(firstEntry, "data2");
  data2_data.load();

  size_t numberOfHistograms =
      static_cast<size_t>(data2_data.dim0() * data2_data.dim1() + data1_data.dim0() * data1_data.dim1()) +
      m_numberOfMonitors;

  int numberOfChannels, numberOfPixelsPerTubeCenter, numberOfTubesCenter;
  getDataDimensions(data1_data, numberOfChannels, numberOfTubesCenter, numberOfPixelsPerTubeCenter);

  MultichannelType type = (numberOfChannels != 1) ? MultichannelType::KINETIC : MultichannelType::TOF;

  createEmptyWorkspace(numberOfHistograms, numberOfChannels, type);
  loadMetaData(firstEntry, instrumentPath);

  // we need to adjust the default binning after loadmetadata
  if (numberOfChannels != 1) {
    std::vector<double> frames(numberOfChannels, 0);
    for (int i = 0; i < numberOfChannels; ++i) {
      frames[i] = i;
    }
    m_defaultBinning.resize(numberOfChannels);
    std::copy(frames.cbegin(), frames.cend(), m_defaultBinning.begin());
  }

  runLoadInstrument();

  const std::string backIndex = m_localWorkspace->getInstrument()->getStringParameter("back_detector_index")[0];
  size_t nextIndex;
  if (backIndex == "2") {
    nextIndex = loadDataFromTubes(data2_data, m_defaultBinning, 0, type);
    nextIndex = loadDataFromTubes(data1_data, m_defaultBinning, nextIndex, type);
  } else {
    nextIndex = loadDataFromTubes(data1_data, m_defaultBinning, 0, type);
    nextIndex = loadDataFromTubes(data2_data, m_defaultBinning, nextIndex, type);
  }
  nextIndex = loadDataFromMonitors(firstEntry, nextIndex, type);

  // hijack the second monitor spectrum to store per-frame durations to enable time normalisation
  if (numberOfChannels != 1) {
    NXFloat durations = firstEntry.openNXFloat("slices");
    durations.load();
    const HistogramData::Counts histoCounts(durations(), durations() + numberOfChannels);
    m_localWorkspace->setCounts(nextIndex - 1, std::move(histoCounts));
    m_localWorkspace->setCountVariances(nextIndex - 1,
                                        HistogramData::CountVariances(std::vector<double>(numberOfChannels, 0)));
  }
}

/**
 * Loads data for D33
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS::initWorkSpaceD33(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {

  g_log.debug("Fetching data...");

  auto dataRear = LoadHelper::getIntDataset(firstEntry, "data1");
  dataRear.load();
  auto dataRight = LoadHelper::getIntDataset(firstEntry, "data2");
  dataRight.load();
  auto dataLeft = LoadHelper::getIntDataset(firstEntry, "data3");
  dataLeft.load();
  auto dataDown = LoadHelper::getIntDataset(firstEntry, "data4");
  dataDown.load();
  auto dataUp = LoadHelper::getIntDataset(firstEntry, "data5");
  dataUp.load();
  g_log.debug("Checking channel numbers...");

  // check number of channels
  if (dataRear.dim2() != dataRight.dim2() && dataRight.dim2() != dataLeft.dim2() &&
      dataLeft.dim2() != dataDown.dim2() && dataDown.dim2() != dataUp.dim2()) {
    throw std::runtime_error("The time bins have not the same dimension for all the 5 detectors!");
  }
  const auto numberOfHistograms = static_cast<size_t>(
      dataRear.dim0() * dataRear.dim1() + dataRight.dim0() * dataRight.dim1() + dataLeft.dim0() * dataLeft.dim1() +
      dataDown.dim0() * dataDown.dim1() + dataUp.dim0() * dataUp.dim1());

  g_log.debug("Creating empty workspace...");
  createEmptyWorkspace(numberOfHistograms + m_numberOfMonitors, static_cast<size_t>(dataRear.dim2()));

  loadMetaData(firstEntry, instrumentPath);

  std::vector<double> binningRear, binningRight, binningLeft, binningDown, binningUp;

  if (firstEntry.getFloat("mode") == 0.0) { // Not TOF
    g_log.debug("Getting default wavelength bins...");
    binningRear = m_defaultBinning;
    binningRight = m_defaultBinning;
    binningLeft = m_defaultBinning;
    binningDown = m_defaultBinning;
    binningUp = m_defaultBinning;

  } else { // TOF
    m_isTOF = true;
    NXInt masterPair = firstEntry.openNXInt(m_instrumentName + "/tof/master_pair");
    masterPair.load();

    const std::string first = std::to_string(masterPair[0]);
    const std::string second = std::to_string(masterPair[1]);
    g_log.debug("Master choppers are " + first + " and " + second);

    NXFloat firstChopper = firstEntry.openNXFloat(m_instrumentName + "/chopper" + first + "/sample_distance");
    firstChopper.load();
    NXFloat secondChopper = firstEntry.openNXFloat(m_instrumentName + "/chopper" + second + "/sample_distance");
    secondChopper.load();
    m_sourcePos = (firstChopper[0] + secondChopper[0]) / 2.;
    g_log.debug("Source distance computed, moving moderator to Z=-" + std::to_string(m_sourcePos));
    g_log.debug("Getting wavelength bins from the nexus file...");
    bool vtof = true;
    // try VTOF mode
    try {
      NXInt channelWidthSum = firstEntry.openNXInt(m_instrumentName + "/tof/chwidth_sum");
      NXFloat channelWidthTimes = firstEntry.openNXFloat(m_instrumentName + "/tof/chwidth_times");
      channelWidthSum.load();
      channelWidthTimes.load();
      std::string distancePrefix(instrumentPath + "/tof/tof_distance_detector");
      binningRear = getVariableTimeBinning(firstEntry, distancePrefix + "1", channelWidthSum, channelWidthTimes);
      binningRight = getVariableTimeBinning(firstEntry, distancePrefix + "2", channelWidthSum, channelWidthTimes);
      binningLeft = getVariableTimeBinning(firstEntry, distancePrefix + "3", channelWidthSum, channelWidthTimes);
      binningDown = getVariableTimeBinning(firstEntry, distancePrefix + "4", channelWidthSum, channelWidthTimes);
      binningUp = getVariableTimeBinning(firstEntry, distancePrefix + "5", channelWidthSum, channelWidthTimes);
    } catch (const std::runtime_error &) {
      vtof = false;
    }
    if (!vtof) {
      try {
        // LTOF mode
        std::string binPathPrefix(instrumentPath + "/tof/tof_wavelength_detector");
        binningRear = LoadHelper::getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "1");
        binningRight = LoadHelper::getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "2");
        binningLeft = LoadHelper::getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "3");
        binningDown = LoadHelper::getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "4");
        binningUp = LoadHelper::getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "5");
      } catch (std::runtime_error &e) {
        throw std::runtime_error("Unable to load the wavelength axes for TOF data " + std::string(e.what()));
      }
    }
  }

  g_log.debug("Loading the data into the workspace...");

  size_t nextIndex = loadDataFromTubes(dataRear, binningRear, 0);
  nextIndex = loadDataFromTubes(dataRight, binningRight, nextIndex);
  nextIndex = loadDataFromTubes(dataLeft, binningLeft, nextIndex);
  nextIndex = loadDataFromTubes(dataDown, binningDown, nextIndex);
  nextIndex = loadDataFromTubes(dataUp, binningUp, nextIndex);
  loadDataFromMonitors(firstEntry, nextIndex);
}

/**
 * @brief Loads data from all the monitors
 * @param firstEntry : already opened first entry in nexus
 * @param firstIndex : the workspace index to load the first monitor to
 * @param type : used to discrimante between TOF and Kinetic
 * @return the next ws index after all the monitors
 */
size_t LoadILLSANS::loadDataFromMonitors(NeXus::NXEntry &firstEntry, size_t firstIndex, const MultichannelType type) {

  // let's find the monitors; should be monitor1 and monitor2
  for (std::vector<NXClassInfo>::const_iterator it = firstEntry.groups().begin(); it != firstEntry.groups().end();
       ++it) {
    if (it->nxclass == "NXmonitor") {
      auto data = LoadHelper::getIntDataset(firstEntry, it->nxname);
      data.load();
      g_log.debug() << "Monitor: " << it->nxname << " dims = " << data.dim0() << "x" << data.dim1() << "x"
                    << data.dim2() << '\n';
      std::vector<double> binning(data.dim2());
      bool pointData;
      if (m_isTOF) {
        binning.push_back(0.0); // bin edges require size to include one more value
        std::iota(binning.begin(), binning.end(), 0.0);
        pointData = false;
      } else {
        binning = m_defaultBinning;
        pointData = type == MultichannelType::KINETIC;
      }
      LoadHelper::fillStaticWorkspace(m_localWorkspace, data, binning, static_cast<int>(firstIndex), pointData);
      // Add average monitor counts to a property:
      double averageMonitorCounts =
          std::accumulate(data(), data() + data.dim2(), double(0)) / static_cast<double>(data.dim2());
      // make sure the monitor has values!
      if (averageMonitorCounts > 0) {
        API::Run &runDetails = m_localWorkspace->mutableRun();
        runDetails.addProperty("monitor", averageMonitorCounts, true);
      }
      firstIndex++;
    }
  }
  return firstIndex;
}

/**
 * @brief Load data from D16B's monitor. These data are not stored in the usual NXmonitor field, but rather in the
 * scanned variables, so it uses a completely different logic.
 * @param firstEntry: already opened first entry in nexus
 * @param firstIndex: the workspace index to start loading the monitor in
 * @param binning: the binning to assign the monitor values
 * @return the new workspace index on which to load next
 */
size_t LoadILLSANS::loadDataFromD16ScanMonitors(const NeXus::NXEntry &firstEntry, size_t firstIndex,
                                                const std::vector<double> &binning) {
  std::string path = "/data_scan/scanned_variables/data";
  // It is not possible to ensure that monitors are in the same position in the scanned_variables data table.
  // Therefore, the order is obtained by getting monitor indices explicitly via getMonitorIndices method.
  auto scannedVariables = LoadHelper::getDoubleDataset(firstEntry, path);
  scannedVariables.load();
  auto monitorNumber = 1; // for the naming of sample log variable
  for (auto monitorIndex : m_monitorIndices) {
    auto firstMonitorValuePos = scannedVariables() + monitorIndex * scannedVariables.dim1();
    const HistogramData::Counts counts(firstMonitorValuePos, firstMonitorValuePos + scannedVariables.dim1());
    m_localWorkspace->setCounts(firstIndex, counts);

    if ((m_instrumentName == "D16" || m_instrumentName == "D16B") && scannedVariables.dim1() == 1) {
      // This is the old D16 data scan format, which also covers single-scan D16B data. It is pain.
      // Due to the fact it was verified with a data structure using binedges rather than points for the wavelength,
      // we have to keep that and not make it an histogram, because some algorithms later in the reduction process
      // handle errors completely differently in this case.
      // We distinguish it from D16 data in the new format but checking there is only one slice of the omega scan
      const HistogramData::BinEdges binEdges(binning);
      m_localWorkspace->setBinEdges(firstIndex, binEdges);
    } else {
      HistogramData::Points points = HistogramData::Points(binning);
      m_localWorkspace->setPoints(firstIndex, points);
    }

    // Add average monitor counts to a property:
    const auto averageMonitorCounts =
        std::accumulate(firstMonitorValuePos, firstMonitorValuePos + scannedVariables.dim1(), double(0)) /
        static_cast<double>(scannedVariables.dim1());

    // make sure the monitor has values!
    if (averageMonitorCounts > 0) {
      API::Run &runDetails = m_localWorkspace->mutableRun();
      runDetails.addProperty("monitor" + std::to_string(monitorNumber), averageMonitorCounts, true);
    }
    monitorNumber++;
    firstIndex++;
    if (m_instrumentName == "D16") {
      // the old D16 has 2 monitors, the second being empty but still needing a binning

      if (scannedVariables.dim1() == 1) {
        const HistogramData::BinEdges binEdges(binning);
        m_localWorkspace->setBinEdges(firstIndex, binEdges);
      } else {
        HistogramData::Points points = HistogramData::Points(binning);
        m_localWorkspace->setPoints(firstIndex, points);
      }
      firstIndex++;
    }
  }
  return firstIndex;
}

/**
 * @brief Loads data from tubes in scan both mode (channels - tubes - pixels) (D16B)
 * @param data : a reference to already loaded nexus data block
 * @param timeBinning : the x-axis binning
 * @param firstIndex : the workspace index to start loading to
 * @param type : used to discrimante between TOF and Kinetic
 * @return the next ws index after all the tubes in the given detector bank
 */
size_t LoadILLSANS::loadDataFromTubes(NeXus::NXInt &data, const std::vector<double> &timeBinning, size_t firstIndex,
                                      const MultichannelType type) {

  int numberOfTubes, numberOfChannels, numberOfPixelsPerTube;
  getDataDimensions(data, numberOfChannels, numberOfTubes, numberOfPixelsPerTube);

  bool pointData = true;
  std::tuple<short, short, short> dimOrder;
  if (m_isD16Omega) {
    dimOrder = std::tuple<short, short, short>{1, 2, 0}; // channels (scans) - tubes - pixels
    if ((m_instrumentName == "D16" || m_instrumentName == "D16B") && numberOfChannels == 1) { // D16 omega scan data
      pointData = false;
    } else { // D16B data
      pointData = true;
    }
  } else {
    pointData = type == MultichannelType::KINETIC;
    dimOrder = std::tuple<short, short, short>{0, 1, 2}; // default, tubes-pixels-channels
  }
  LoadHelper::fillStaticWorkspace(m_localWorkspace, data, timeBinning, static_cast<int>(firstIndex), pointData,
                                  std::vector<int>(), std::set<int>(), dimOrder);
  return firstIndex + numberOfTubes * numberOfPixelsPerTube;
}

/**
 * Create a workspace without any data in it
 * @param numberOfHistograms : number of spectra
 * @param numberOfChannels : number of TOF channels
 * @param type : type of the multichannel workspace (TOF (default) or Kinetic)
 */
void LoadILLSANS::createEmptyWorkspace(const size_t numberOfHistograms, const size_t numberOfChannels,
                                       const MultichannelType type) {
  const size_t numberOfElementsInX = numberOfChannels + ((type == MultichannelType::TOF && !m_isD16Omega) ? 1 : 0);
  m_localWorkspace =
      WorkspaceFactory::Instance().create("Workspace2D", numberOfHistograms, numberOfElementsInX, numberOfChannels);

  switch (type) {
  case MultichannelType::TOF:
    m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    break;
  case MultichannelType::SCAN: {
    auto labelX = std::dynamic_pointer_cast<Kernel::Units::Label>(Kernel::UnitFactory::Instance().create("Label"));
    labelX->setLabel("Omega angle");
    m_localWorkspace->getAxis(0)->unit() = labelX;
    break;
  }
  case MultichannelType::KINETIC:
    break;
  }

  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Makes up the full path of the relevant IDF dependent on resolution mode
 * @param instName : the name of the instrument (including the resolution mode
 * suffix)
 * @return : the full path to the corresponding IDF
 */
std::string LoadILLSANS::getInstrumentFilePath(const std::string &instName) const {

  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(instName + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

/**
 * Loads the instrument from the IDF
 */
void LoadILLSANS::runLoadInstrument() {
  std::string instrumentPath = m_instrumentName;
  if (m_resMode == "low") {
    // low resolution mode we have only defined for the old D11 and D22
    instrumentPath += "lr";
  }
  instrumentPath += "_Definition.xml";
  LoadHelper::loadEmptyInstrument(m_localWorkspace, m_instrumentName, instrumentPath);
}

/**
 * Move the detector banks for D33
 * @param detPos : structure holding the positions
 */
void LoadILLSANS::moveDetectorsD33(const DetectorPosition &detPos) {
  // Move in Z
  moveDetectorDistance(detPos.distanceSampleRear, "back_detector");
  moveDetectorDistance(detPos.distanceSampleBottomTop, "front_detector_top");
  moveDetectorDistance(detPos.distanceSampleBottomTop, "front_detector_bottom");
  moveDetectorDistance(detPos.distanceSampleRightLeft, "front_detector_right");
  moveDetectorDistance(detPos.distanceSampleRightLeft, "front_detector_left");
  // Move in X
  moveDetectorHorizontal(detPos.shiftLeft, "front_detector_left");
  moveDetectorHorizontal(-detPos.shiftRight, "front_detector_right");
  // Move in Y
  moveDetectorVertical(detPos.shiftUp, "front_detector_top");
  moveDetectorVertical(-detPos.shiftDown, "front_detector_bottom");
  // Set the sample log
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty<double>("L2", detPos.distanceSampleRear, true);
}

/**
 * Move detectors in Z axis (X,Y are kept constant)
 * @param distance : the distance to move along Z axis [meters]
 * @param componentName : name of the component to move
 */
void LoadILLSANS::moveDetectorDistance(double distance, const std::string &componentName) {

  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = LoadHelper::getComponentPosition(m_localWorkspace, componentName);
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);

  mover->setProperty("X", pos.X());
  mover->setProperty("Y", pos.Y());
  mover->setProperty("Z", distance);
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();

  g_log.debug() << "Moving component '" << componentName << "' to Z = " << distance << '\n';
}

/**
 * Rotates instrument detector around y-axis in place
 * @param angle : the angle to rotate [degree]
 * @param componentName : "detector"
 */
void LoadILLSANS::rotateInstrument(double angle, const std::string &componentName) {
  auto rotater = createChildAlgorithm("RotateInstrumentComponent");
  rotater->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  rotater->setProperty("ComponentName", componentName);
  rotater->setProperty("X", 0.);
  rotater->setProperty("Y", 1.);
  rotater->setProperty("Z", 0.);
  rotater->setProperty("Angle", angle);
  rotater->setProperty("RelativeRotation", false);
  rotater->executeAsChildAlg();
  g_log.debug() << "Rotating component '" << componentName << "' to angle = " << angle << " degrees.\n";
}

/**
 * @brief LoadILLSANS::placeD16 : place the D16 detector.
 * @param angle : the angle between its center and the transmitted beam
 * @param distance : the distance between its center and the sample
 * @param componentName : "detector"
 */
void LoadILLSANS::placeD16(double angle, double distance, const std::string &componentName) {
  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);
  mover->setProperty("X", sin(angle * M_PI / 180) * distance);
  mover->setProperty("Y", 0.);
  mover->setProperty("Z", cos(angle * M_PI / 180) * distance);
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();

  // rotate the detector so it faces the sample.
  rotateInstrument(angle, componentName);
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty<double>("L2", distance, true);

  g_log.debug() << "Moving component '" << componentName << "' to angle = " << angle
                << " degrees and distance = " << distance << "metres.\n";
}

/**
 * Move detectors in X
 * @param shift : the distance to move [metres]
 * @param componentName : the name of the component
 */
void LoadILLSANS::moveDetectorHorizontal(double shift, const std::string &componentName) {
  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = LoadHelper::getComponentPosition(m_localWorkspace, componentName);
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);
  mover->setProperty("X", shift);
  mover->setProperty("Y", pos.Y());
  mover->setProperty("Z", pos.Z());
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();
  g_log.debug() << "Moving component '" << componentName << "' to X = " << shift << '\n';
}

/**
 * Move detectors in Y
 * @param shift : the distance to move [metres]
 * @param componentName : the name of the component
 */
void LoadILLSANS::moveDetectorVertical(double shift, const std::string &componentName) {
  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = LoadHelper::getComponentPosition(m_localWorkspace, componentName);
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);
  mover->setProperty("X", pos.X());
  mover->setProperty("Y", shift);
  mover->setProperty("Z", pos.Z());
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();
  g_log.debug() << "Moving component '" << componentName << "' to Y = " << shift << '\n';
}

/**
 * Loads some metadata present in the nexus file
 * @param entry : opened nexus entry
 * @param instrumentNamePath : the nexus entry of the instrument
 */
void LoadILLSANS::loadMetaData(const NeXus::NXEntry &entry, const std::string &instrumentNamePath) {

  g_log.debug("Loading metadata...");
  API::Run &runDetails = m_localWorkspace->mutableRun();

  if ((entry.getFloat("mode") == 0.0) || (m_instrumentName == "D16")) { // Not TOF
    runDetails.addProperty<std::string>("tof_mode", "Non TOF");
  } else {
    runDetails.addProperty<std::string>("tof_mode", "TOF");
  }

  double wavelength;
  if (getPointerToProperty("Wavelength")->isDefault()) {
    if (m_instrumentName == "D16" || m_instrumentName == "D16B") {
      wavelength = entry.getFloat(instrumentNamePath + "/Beam/wavelength");
    } else {
      wavelength = entry.getFloat(instrumentNamePath + "/selector/wavelength");
    }
    g_log.debug() << "Wavelength found in the nexus file: " << wavelength << '\n';
  } else {
    wavelength = getProperty("Wavelength");
  }

  // round the wavelength to avoid unnecessary rebinning during merge runs
  wavelength = std::round(wavelength * 100) / 100.;

  if (wavelength <= 0) {
    g_log.debug() << "Mode = " << entry.getFloat("mode") << '\n';
    g_log.information("The wavelength present in the NeXus file <= 0.");
    if (entry.getFloat("mode") == 0.0) { // Not TOF
      throw std::runtime_error("Working in Non TOF mode and the wavelength in "
                               "the file is <=0 !!! Check with the instrument "
                               "scientist!");
    }
  } else {
    double wavelengthRes = 10.;
    const std::string entryResolution = instrumentNamePath + "/selector/";
    try {
      wavelengthRes = entry.getFloat(entryResolution + "wavelength_res");
    } catch (const std::runtime_error &) {
      try {
        wavelengthRes = entry.getFloat(entryResolution + "wave_length_res");
      } catch (const std::runtime_error &) {
        if (m_instrumentName == "D16")
          wavelengthRes = 1;
        g_log.information() << "Could not find wavelength resolution, assuming " << wavelengthRes << "%.\n";
      }
    }
    // round also the wavelength res to avoid unnecessary rebinning during
    // merge runs
    wavelengthRes = std::round(wavelengthRes * 100) / 100.;
    runDetails.addProperty<double>("wavelength", wavelength);
    double ei = LoadHelper::calculateEnergy(wavelength);
    runDetails.addProperty<double>("Ei", ei, true);
    // wavelength
    m_defaultBinning[0] = wavelength - wavelengthRes * wavelength * 0.01 / 2;
    m_defaultBinning[1] = wavelength + wavelengthRes * wavelength * 0.01 / 2;
  }

  // the start time is needed in the workspace when loading the parameter file
  std::string startDate = entry.getString("start_time");
  runDetails.addProperty<std::string>("start_time", LoadHelper::dateTimeInIsoFormat(startDate));
  // set the facility
  runDetails.addProperty<std::string>("Facility", std::string("ILL"));
}

/**
 * Sets full sample logs
 * @param filename : name of the file
 */
void LoadILLSANS::setFinalProperties(const std::string &filename) {
  API::Run &runDetails = m_localWorkspace->mutableRun();
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(filename.c_str(), NXACC_READ, &nxHandle);
  if (nxStat != NXstatus::NX_ERROR) {
    LoadHelper::addNexusFieldsToWsRun(nxHandle, runDetails);
    NXclose(&nxHandle);
  }
}

/**
 * Adjusts pixel by pixel the wavelength axis
 * Used only for D33 in TOF mode
 */
void LoadILLSANS::adjustTOF() {
  const auto &specInfo = m_localWorkspace->spectrumInfo();
  const double l1 = m_sourcePos;
  const size_t nHist = m_localWorkspace->getNumberHistograms();
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_localWorkspace))
  for (int64_t index = 0; index < static_cast<int64_t>(nHist - m_numberOfMonitors); ++index) {
    const double l2 = specInfo.l2(index);
    const double z = specInfo.position(index).Z();
    auto &x = m_localWorkspace->mutableX(index);
    const double scale = (l1 + z) / (l1 + l2);
    std::transform(x.begin(), x.end(), x.begin(), [scale](double lambda) { return scale * lambda; });
  }

  // Try to set sensible (but not strictly physical) wavelength axes for monitors
  // Normalisation is done by acquisition time, so these axes should not be used
  auto firstPixel = m_localWorkspace->histogram(0).dataX();
  const double l2 = specInfo.l2(0);
  const double monitor2 = -specInfo.position(nHist - 1).Z();
  const double l1Monitor2 = m_sourcePos - monitor2;
  const double monScale = (l1 + l2) / l1Monitor2;
  std::transform(firstPixel.begin(), firstPixel.end(), firstPixel.begin(),
                 [monScale](double lambda) { return monScale * lambda; });
  for (size_t mIndex = nHist - m_numberOfMonitors; mIndex < nHist; ++mIndex) {
    const HistogramData::Counts counts = m_localWorkspace->histogram(mIndex).counts();
    const HistogramData::BinEdges binEdges(firstPixel);
    m_localWorkspace->setHistogram(mIndex, binEdges, counts);
  }
}

/**
 * Moves the source to the middle of the two master choppers
 * Used only for D33 in TOF mode
 */
void LoadILLSANS::moveSource() {
  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", "moderator");
  mover->setProperty("X", 0.);
  mover->setProperty("Y", 0.);
  mover->setProperty("Z", -m_sourcePos);
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();
}

/**
 * @brief LoadILLSANS::getOmegaBinning
 * Get the binning for an omega scan for D16B files
 * @param entry opened root nexus entry
 * @param path path of the scanned variables entry
 * @return the omega binning vector
 */
std::vector<double> LoadILLSANS::getOmegaBinning(const NXEntry &entry, const std::string &path) const {

  auto scannedValues = LoadHelper::getDoubleDataset(entry, path);
  scannedValues.load();

  const int nBins = scannedValues.dim1();
  std::vector<double> binning(nBins, 0);

  for (int i = 0; i < nBins; ++i) {
    // for D16, we are only interested in the first line, which contains the omega values
    binning[i] = scannedValues(0, i);
  }
  return binning;
}

/**
 * Returns the wavelength axis computed in VTOF mode
 * @param entry : opened root nexus entry
 * @param path : path of the detector distance entry
 * @param sum : loaded channel width sums
 * @param times : loaded channel width times
 * @return binning : wavelength bin boundaries
 */
std::vector<double> LoadILLSANS::getVariableTimeBinning(const NXEntry &entry, const std::string &path, const NXInt &sum,
                                                        const NXFloat &times) const {
  const int nBins = sum.dim0();
  std::vector<double> binCenters;
  binCenters.reserve(nBins);
  NXFloat distance = entry.openNXFloat(path);
  distance.load();
  for (int bin = 0; bin < nBins; ++bin) {
    // sum is in nanoseconds, times is in microseconds
    const double tof = sum[bin] * 1E-9 - times[bin] * 1E-6 / 2.;
    // velocity in m/s
    const double velocity = distance[0] / tof;
    // wavelength in AA
    const double lambda = PhysicalConstants::h / PhysicalConstants::NeutronMass / velocity * 1E+10;
    binCenters.emplace_back(lambda);
  }
  std::vector<double> binEdges;
  binEdges.reserve(nBins + 1);
  VectorHelper::convertToBinBoundary(binCenters, binEdges);
  // after conversion to bin edges, the first item might get negative,
  // which is not physical, set to 0
  if (binEdges[0] < 0.) {
    binEdges[0] = 0.;
  }
  return binEdges;
}

} // namespace Mantid::DataHandling
