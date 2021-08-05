// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLSANS2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

namespace {
static constexpr size_t N_MONITORS = 2;
}

using namespace Kernel;
using namespace API;
using namespace NeXus;

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLSANS2)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLSANS2::LoadILLSANS2()
    : m_supportedInstruments{"D11", "D22", "D33", "D16"}, m_defaultBinning{0}, m_resMode("nominal"),
      m_sourcePos(0.), m_isD16Omega{false}, m_loadInstrument{true} {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLSANS2::name() const { return "LoadILLSANS"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLSANS2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLSANS2::category() const { return "DataHandling\\Nexus;ILL\\SANS"; }

/// Algorithm's summary. @see Algorithm::summery
const std::string LoadILLSANS2::summary() const {
  return "Loads ILL nexus files for SANS instruments D11, D16, D22, D33.";
}

//----------------------------------------------------------------------------------------------

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLSANS2::confidence(Kernel::NexusDescriptor &descriptor) const {
  // fields existent only at the ILL for SANS machines
  if (descriptor.pathExists("/entry0/mode") &&
      ((descriptor.pathExists("/entry0/reactor_power") && descriptor.pathExists("/entry0/instrument_name")) ||
       (descriptor.pathExists("/entry0/instrument/name") && descriptor.pathExists("/entry0/acquisition_mode") &&
        !descriptor.pathExists("/entry0/instrument/Detector")))) // serves to remove the TOF
                                                                 // instruments
  {
    return 80;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLSANS2::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "Name of the nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
  declareProperty("LoadInstrument", true, "Whether to load the instrument geometry with the data.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLSANS2::exec() {
  m_loadInstrument = getProperty("LoadInstrument");
  const std::string filename = getPropertyValue("Filename");
  NXRoot root(filename);
  NXEntry firstEntry = root.openFirstEntry();
  const std::string instrumentPath = m_loadHelper.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);
  figureOutMeasurementType(firstEntry);
  Progress progress(this, 0.0, 1.0, 4);
  progress.report("Initializing the workspace for " + m_instrumentName);
  initWorkspace(firstEntry, instrumentPath);
  if (getProperty("LoadInstrument")) {
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();
    progress.report("Placing the instrument " + m_instrumentName);
    placeInstrument(firstEntry, instrumentPath);
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
void LoadILLSANS2::setInstrumentName(const NeXus::NXEntry &firstEntry, const std::string &instrumentNamePath) {
  if (instrumentNamePath.empty()) {
    throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
  }
  m_instrumentName = m_loadHelper.getStringFromNexusPath(firstEntry, instrumentNamePath + "/name");
  const auto inst = std::find(m_supportedInstruments.begin(), m_supportedInstruments.end(), m_instrumentName);

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
 * Get detector panel distances from the nexus file
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentNamePath : the path inside nexus where the instrument name is written
 * @return a structure with the positions
 */
LoadILLSANS2::DetectorPosition LoadILLSANS2::getDetectorPositionD33(const NeXus::NXEntry &firstEntry,
                                                                    const std::string &instrumentNamePath) {
  std::string detectorPath(instrumentNamePath + "/detector");
  DetectorPosition pos;
  pos.distanceSampleRear = m_loadHelper.getDoubleFromNexusPath(firstEntry, detectorPath + "/det2_calc");
  pos.distanceSampleBottomTop = m_loadHelper.getDoubleFromNexusPath(firstEntry, detectorPath + "/det1_calc");
  pos.distanceSampleRightLeft = pos.distanceSampleBottomTop + m_loadHelper.getDoubleFromNexusPath(
                                                                  firstEntry, detectorPath + "/det1_panel_separation");
  pos.shiftLeft = m_loadHelper.getDoubleFromNexusPath(firstEntry, detectorPath + "/OxL_actual") * 1e-3;
  pos.shiftRight = m_loadHelper.getDoubleFromNexusPath(firstEntry, detectorPath + "/OxR_actual") * 1e-3;
  pos.shiftUp = m_loadHelper.getDoubleFromNexusPath(firstEntry, detectorPath + "/OyT_actual") * 1e-3;
  pos.shiftDown = m_loadHelper.getDoubleFromNexusPath(firstEntry, detectorPath + "/OyB_actual") * 1e-3;
  return pos;
}

/**
 * Loads data for all supported instruments
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS2::initWorkspace(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {
  size_t firstMonitorIndex = 0;
  std::vector<std::string> defaultInstruments{"D11", "D16", "D22"};
  if (std::find(defaultInstruments.begin(), defaultInstruments.end(), m_instrumentName) != defaultInstruments.end()) {
    std::string path;
    if (firstEntry.containsGroup("data")) {
      path = "data";
    } else {
      path = "data_scan/detector_data/data";
    }
    NXData dataGroup = firstEntry.openNXData(path);
    NXInt data = dataGroup.openIntData();
    data.load();
    size_t numberOfHistograms;

    if (m_isD16Omega) {
      numberOfHistograms = static_cast<size_t>(data.dim1() * data.dim2()) + N_MONITORS;
    } else {
      numberOfHistograms = static_cast<size_t>(data.dim0() * data.dim1()) + N_MONITORS;
    }
    createEmptyWorkspace(numberOfHistograms, 1);

    firstMonitorIndex = loadDataFromTubes(data, m_defaultBinning, 0);
    if (data.dim1() == 128) {
      m_resMode = "low";
    }
  } else if (m_instrumentName == "D11B") {
    firstMonitorIndex = initWorkspaceD11B(firstEntry);
  } else if (m_instrumentName == "D22B") {
    firstMonitorIndex = initWorkspaceD22B(firstEntry);
  } else if (m_instrumentName == "D33") {
    firstMonitorIndex = initWorkspaceD33(firstEntry, instrumentPath);
  }
  loadMetaData(firstEntry, instrumentPath);
  loadDataFromMonitors(firstEntry, firstMonitorIndex);
}

/**
 * Loads D11B data
 * @param firstEntry : already opened first entry in nexus
 */
size_t LoadILLSANS2::initWorkspaceD11B(NeXus::NXEntry &firstEntry) {
  g_log.debug("Fetching data...");

  NXData data1 = firstEntry.openNXData("D11/Detector 1/data");
  NXInt dataCenter = data1.openIntData();
  dataCenter.load();
  NXData data2 = firstEntry.openNXData("D11/Detector 2/data");
  NXInt dataLeft = data2.openIntData();
  dataLeft.load();
  NXData data3 = firstEntry.openNXData("D11/Detector 3/data");
  NXInt dataRight = data3.openIntData();
  dataRight.load();

  size_t numberOfHistograms =
      static_cast<size_t>(dataCenter.dim0() * dataCenter.dim1() + dataRight.dim0() * dataRight.dim1() +
                          dataLeft.dim0() * dataLeft.dim1()) +
      N_MONITORS;

  createEmptyWorkspace(numberOfHistograms, dataCenter.dim2());
  size_t nextIndex;
  nextIndex = loadDataFromTubes(dataCenter, m_defaultBinning, 0);
  nextIndex = loadDataFromTubes(dataLeft, m_defaultBinning, nextIndex);
  nextIndex = loadDataFromTubes(dataRight, m_defaultBinning, nextIndex);
  return nextIndex;
}

/**
 * Initializes empty instrument and loads D22B data
 * @param firstEntry : already opened first entry in nexus
 */
size_t LoadILLSANS2::initWorkspaceD22B(NeXus::NXEntry &firstEntry) {

  NXData data2 = firstEntry.openNXData("data2");
  NXInt data2_data = data2.openIntData();
  data2_data.load();
  NXData data1 = firstEntry.openNXData("data1");
  NXInt data1_data = data1.openIntData();
  data1_data.load();

  size_t numberOfHistograms =
      static_cast<size_t>(data2_data.dim0() * data2_data.dim1() + data1_data.dim0() * data1_data.dim1()) + N_MONITORS;

  createEmptyWorkspace(numberOfHistograms, 1);
  runLoadInstrument(); // necessary for D22B to extract back_detector_index

  const std::string backIndex = m_localWorkspace->getInstrument()->getStringParameter("back_detector_index")[0];
  size_t nextIndex;
  if (backIndex == "2") {
    nextIndex = loadDataFromTubes(data2_data, m_defaultBinning, 0);
    nextIndex = loadDataFromTubes(data1_data, m_defaultBinning, nextIndex);
  } else {
    nextIndex = loadDataFromTubes(data1_data, m_defaultBinning, 0);
    nextIndex = loadDataFromTubes(data2_data, m_defaultBinning, nextIndex);
  }
  return nextIndex;
}

/**
 * Loads data for D33
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
size_t LoadILLSANS2::initWorkspaceD33(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {

  NXData dataGroup1 = firstEntry.openNXData("data1");
  NXInt dataRear = dataGroup1.openIntData();
  dataRear.load();
  NXData dataGroup2 = firstEntry.openNXData("data2");
  NXInt dataRight = dataGroup2.openIntData();
  dataRight.load();
  NXData dataGroup3 = firstEntry.openNXData("data3");
  NXInt dataLeft = dataGroup3.openIntData();
  dataLeft.load();
  NXData dataGroup4 = firstEntry.openNXData("data4");
  NXInt dataDown = dataGroup4.openIntData();
  dataDown.load();
  NXData dataGroup5 = firstEntry.openNXData("data5");
  NXInt dataUp = dataGroup5.openIntData();
  dataUp.load();

  // check number of channels
  if (dataRear.dim2() != dataRight.dim2() && dataRight.dim2() != dataLeft.dim2() &&
      dataLeft.dim2() != dataDown.dim2() && dataDown.dim2() != dataUp.dim2()) {
    throw std::runtime_error("The time bins have not the same dimension for all the 5 detectors!");
  }
  const auto numberOfHistograms = static_cast<size_t>(
      dataRear.dim0() * dataRear.dim1() + dataRight.dim0() * dataRight.dim1() + dataLeft.dim0() * dataLeft.dim1() +
      dataDown.dim0() * dataDown.dim1() + dataUp.dim0() * dataUp.dim1());

  createEmptyWorkspace(numberOfHistograms + N_MONITORS, static_cast<size_t>(dataRear.dim2()));

  std::vector<double> binningRear, binningRight, binningLeft, binningDown, binningUp;

  if (m_measurementType == MeasurementType::MONO) { // Not TOF
    binningRear = m_defaultBinning;
    binningRight = m_defaultBinning;
    binningLeft = m_defaultBinning;
    binningDown = m_defaultBinning;
    binningUp = m_defaultBinning;

  } else { // TOF
    NXInt masterPair = firstEntry.openNXInt(m_instrumentName + "/tof/master_pair");
    masterPair.load();

    const std::string first = std::to_string(masterPair[0]);
    const std::string second = std::to_string(masterPair[1]);
    NXFloat firstChopper = firstEntry.openNXFloat(m_instrumentName + "/chopper" + first + "/sample_distance");
    firstChopper.load();
    NXFloat secondChopper = firstEntry.openNXFloat(m_instrumentName + "/chopper" + second + "/sample_distance");
    secondChopper.load();
    m_sourcePos = (firstChopper[0] + secondChopper[0]) / 2.;
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
        binningRear = m_loadHelper.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "1");
        binningRight = m_loadHelper.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "2");
        binningLeft = m_loadHelper.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "3");
        binningDown = m_loadHelper.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "4");
        binningUp = m_loadHelper.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "5");
      } catch (std::runtime_error &e) {
        throw std::runtime_error("Unable to load the wavelength axes for TOF data " + std::string(e.what()));
      }
    }
  }

  size_t nextIndex = loadDataFromTubes(dataRear, binningRear, 0);
  nextIndex = loadDataFromTubes(dataRight, binningRight, nextIndex);
  nextIndex = loadDataFromTubes(dataLeft, binningLeft, nextIndex);
  nextIndex = loadDataFromTubes(dataDown, binningDown, nextIndex);
  nextIndex = loadDataFromTubes(dataUp, binningUp, nextIndex);
  return nextIndex;
}

/**
 * @brief Loads data from all the monitors
 * @param firstEntry : already opened first entry in nexus
 * @param firstIndex : the workspace index to load the first monitor to
 * @return the next ws index after all the monitors
 */
size_t LoadILLSANS2::loadDataFromMonitors(NeXus::NXEntry &firstEntry, size_t firstIndex) {

  // let's find the monitors; should be monitor1 and monitor2
  bool monitor1 = true;
  for (auto it = firstEntry.groups().begin(); it != firstEntry.groups().end(); ++it) {
    if (it->nxclass == "NXmonitor") {
      NXData dataGroup = firstEntry.openNXData(it->nxname);
      NXInt data = dataGroup.openIntData();
      data.load();
      HistogramData::Counts histoCounts;
      HistogramData::CountVariances histoVariances;
      if ((monitor1 && m_instrumentName == "D16") ||
          (!monitor1 && m_instrumentName != "D16")) { // This hijacks the empty monitor and fills it with duration,
                                                      // M1 for D16, M2 for D11(B), D22(B), and D33
        NXFloat durations = firstEntry.openNXFloat("duration");
        if (m_measurementType == MeasurementType::KINETIC) {
          durations = firstEntry.openNXFloat("slices");
        }
        durations.load();
        histoCounts = HistogramData::Counts(durations(), durations() + data.dim2());
        histoVariances = HistogramData::CountVariances(std::vector<double>(data.dim2(), 0));
      } else {
        histoCounts = HistogramData::Counts(data(), data() + data.dim2());
        histoVariances = HistogramData::CountVariances(data(), data() + data.dim2());
      }
      m_localWorkspace->setCounts(firstIndex, std::move(histoCounts));
      m_localWorkspace->setCountVariances(firstIndex, std::move(histoVariances));

      if (m_measurementType == MeasurementType::TOF) {
        HistogramData::BinEdges histoBinEdges(data.dim2() + 1, HistogramData::LinearGenerator(0.0, 1));
        m_localWorkspace->setBinEdges(firstIndex, std::move(histoBinEdges));
      } else {
        HistogramData::Points histoPoints = HistogramData::Points(m_defaultBinning);
        m_localWorkspace->setPoints(firstIndex, std::move(histoPoints));
      }

      // Add average monitor counts to a property:
      double averageMonitorCounts = std::accumulate(data(), data() + data.dim2(), 0) / static_cast<double>(data.dim2());
      // make sure the monitor has values!
      if (averageMonitorCounts > 0) {
        API::Run &runDetails = m_localWorkspace->mutableRun();
        runDetails.addProperty("monitor", averageMonitorCounts, true);
      }
      firstIndex++;
      monitor1 = false;
    }
  }
  return firstIndex;
}

/**
 * @brief Loads data from tubes
 * @param data : a reference to already loaded nexus data block
 * @param binning : the x-axis binning
 * @param firstIndex : the workspace index to start loading to
 * @return the next ws index after all the tubes in the given detector bank
 */
size_t LoadILLSANS2::loadDataFromTubes(NeXus::NXInt &data, const std::vector<double> &binning, size_t firstIndex = 0) {
  int numberOfTubes;
  int numberOfChannels;
  const int numberOfPixelsPerTube = data.dim1();

  if (m_isD16Omega) {
    // D16 with omega scan case
    numberOfTubes = data.dim2();
    numberOfChannels = data.dim0();
  } else {
    numberOfTubes = data.dim0();
    numberOfChannels = data.dim2();
  }

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_localWorkspace))
  for (int i = 0; i < numberOfTubes; ++i) {
    for (int j = 0; j < numberOfPixelsPerTube; ++j) {
      int *data_p;
      if (m_isD16Omega) {
        data_p = &data(0, i, j);
      } else {
        data_p = &data(i, j, 0);
      }
      const size_t index = firstIndex + i * numberOfPixelsPerTube + j;
      const HistogramData::Counts histoCounts(data_p, data_p + numberOfChannels);
      const HistogramData::CountVariances histoVariances(data_p, data_p + numberOfChannels);
      m_localWorkspace->setCounts(index, std::move(histoCounts));
      m_localWorkspace->setCountVariances(index, std::move(histoVariances));
      if (m_measurementType == MeasurementType::TOF) {
        const HistogramData::BinEdges binEdges(binning);
        m_localWorkspace->setBinEdges(index, std::move(binEdges));
      } else {
        const HistogramData::Points histoPoints(binning);
        m_localWorkspace->setPoints(index, std::move(histoPoints));
      }
    }
  }

  return firstIndex + numberOfTubes * numberOfPixelsPerTube;
}

/**
 * Create a workspace without any data in it
 * @param numberOfHistograms : number of spectra
 * @param numberOfBins: number of X-axis bins
 */
void LoadILLSANS2::createEmptyWorkspace(const size_t numberOfHistograms, const size_t numberOfBins) {
  const size_t numberOfElementsInX = numberOfBins + (m_measurementType == MeasurementType::TOF ? 1 : 0);
  m_localWorkspace =
      WorkspaceFactory::Instance().create("Workspace2D", numberOfHistograms, numberOfElementsInX, numberOfBins);
  m_localWorkspace->setYUnitLabel("Counts");
  if (m_measurementType == MeasurementType::TOF) {
    m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  }
}

/**
 * Makes up the full path of the relevant IDF dependent on resolution mode
 * @param instName : the name of the instrument (including the resolution mode
 * suffix)
 * @return : the full path to the corresponding IDF
 */
std::string LoadILLSANS2::getInstrumentFilePath(const std::string &instName) const {

  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(instName + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

/**
 * Loads the instrument from the IDF
 */
void LoadILLSANS2::runLoadInstrument() {

  auto loadInst = createChildAlgorithm("LoadInstrument");
  if (m_resMode == "nominal") {
    loadInst->setPropertyValue("Filename", getInstrumentFilePath(m_instrumentName));
  } else if (m_resMode == "low") {
    // low resolution mode we have only defined for the old D11 and D22
    loadInst->setPropertyValue("Filename", getInstrumentFilePath(m_instrumentName + "lr"));
  }
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  loadInst->execute();
}

/**
 * @brief LoadILLSANS2::placeInstrument : places the instrument in the correct 3D position.
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS2::placeInstrument(const NXEntry &firstEntry, const std::string &instrumentPath) {
  double distance = 0;
  if (m_instrumentName == "D33") {
    const DetectorPosition detPos = getDetectorPositionD33(firstEntry, instrumentPath);
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
    if (m_measurementType == MeasurementType::TOF) {
      adjustTOF();
      moveSource();
    }
  } else if (m_instrumentName == "D16") {
    distance = firstEntry.getFloat(instrumentPath + "/Det/value") / 1000; // mm to metre
    const double angle = -1.0 * firstEntry.getFloat(instrumentPath + "/Gamma/value");
    moveDetectorDistance(distance, "detector", angle);
    // rotate the detector so it faces the sample.
    rotateInstrument(angle, "detector");
  } else if (m_instrumentName == "D11B") {
    // we move the parent "detector" component, but since it is at (0,0,0), we
    // need to find the distance it has to move and move it to this position
    distance = firstEntry.getFloat(instrumentPath + "/Detector 1/det_calc");
    V3D pos = getComponentPosition("detector_center");
    double currentDistance = pos.Z();
    moveDetectorDistance(distance - currentDistance, "detector");
  } else if (m_instrumentName == "D22B") {
    const std::string backIndex = m_localWorkspace->getInstrument()->getStringParameter("back_detector_index")[0];
    const std::string frontIndex = m_localWorkspace->getInstrument()->getStringParameter("front_detector_index")[0];

    // first, let's move the front (right) detector
    distance = firstEntry.getFloat(instrumentPath + "/Detector " + frontIndex + "/det" + frontIndex + "_calc");
    moveDetectorDistance(distance, "detector_front");

    // mm to meter
    double offset = firstEntry.getFloat(instrumentPath + "/Detector " + frontIndex + "/dtr" + frontIndex + "_actual");
    moveDetectorHorizontal(-offset / 1000, "detector_front"); // mm to meter
    double angle = firstEntry.getFloat(instrumentPath + "/Detector " + frontIndex + "/dan" + frontIndex + "_actual");
    rotateInstrument(-angle, "detector_front");

    // then, we move the central (back) detector
    distance = firstEntry.getFloat(instrumentPath + "/Detector " + backIndex + "/det" + backIndex + "_calc");
    moveDetectorDistance(distance, "detector_back");

    offset = firstEntry.getFloat(instrumentPath + "/Detector " + backIndex + "/dtr" + backIndex + "_actual");
    moveDetectorHorizontal(-offset / 1000, "detector_back"); // mm to meter

  } else { // D11 and D22
    distance = m_loadHelper.getDoubleFromNexusPath(firstEntry, instrumentPath + "/detector/det_calc");
    moveDetectorDistance(distance, "detector");
    if (m_instrumentName == "D22") {
      double offset = m_loadHelper.getDoubleFromNexusPath(firstEntry, instrumentPath + "/detector/dtr_actual");
      moveDetectorHorizontal(-offset / 1000, "detector"); // mm to meter
    }
  }
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty<double>("L2", distance, true);
}

/**
 * Move detectors in Z axis while keeping other axes untouched or move it at with a specified angle
 * @param distance : the distance instrument center and the sample [meters]
 * @param componentName : name of the component to move
 * @param angle : the angle between instrument center and the transmitted beam
 */
void LoadILLSANS2::moveDetectorDistance(const double distance, const std::string &componentName, const double angle) {

  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);
  if (angle == 0) {
    V3D pos = getComponentPosition(componentName);
    mover->setProperty("X", pos.X());
    mover->setProperty("Y", pos.Y());
    mover->setProperty("Z", distance);
  } else { // used for D16
    mover->setProperty("X", sin(angle * M_PI / 180) * distance);
    mover->setProperty("Y", 0.);
    mover->setProperty("Z", cos(angle * M_PI / 180) * distance);
  }
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();

  g_log.debug() << "Moving component '" << componentName << "' to Z = " << distance << '\n';
}

/**
 * Rotates instrument detector around y-axis in place
 * @param angle : the angle to rotate [degree]
 * @param componentName : "detector"
 */
void LoadILLSANS2::rotateInstrument(double angle, const std::string &componentName) {
  auto rotater = createChildAlgorithm("RotateInstrumentComponent");
  rotater->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  rotater->setProperty("ComponentName", componentName);
  rotater->setProperty("X", 0.);
  rotater->setProperty("Y", 1.);
  rotater->setProperty("Z", 0.);
  rotater->setProperty("Angle", angle);
  rotater->setProperty("RelativeRotation", false);
  rotater->executeAsChildAlg();
}

/**
 * Move detectors in X
 * @param shift : the distance to move [metres]
 * @param componentName : the name of the component
 */
void LoadILLSANS2::moveDetectorHorizontal(double shift, const std::string &componentName) {
  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
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
void LoadILLSANS2::moveDetectorVertical(double shift, const std::string &componentName) {
  auto mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
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
 * Get position of a component
 * @param componentName : the name of the component
 * @return : V3D of the component position
 */
V3D LoadILLSANS2::getComponentPosition(const std::string &componentName) {
  Geometry::Instrument_const_sptr instrument = m_localWorkspace->getInstrument();
  Geometry::IComponent_const_sptr component = instrument->getComponentByName(componentName);
  return component->getPos();
}

/**
 * Loads some metadata present in the nexus file
 * @param entry : opened nexus entry
 * @param instrumentNamePath : the nexus entry of the instrument
 */
void LoadILLSANS2::loadMetaData(const NeXus::NXEntry &entry, const std::string &instrumentNamePath) {

  API::Run &runDetails = m_localWorkspace->mutableRun();

  if ((entry.getFloat("mode") == 0.0) || (m_instrumentName == "D16")) { // Not TOF
    runDetails.addProperty<std::string>("tof_mode", "Non TOF");
  } else {
    runDetails.addProperty<std::string>("tof_mode", "TOF");
  }

  double wavelength;
  if (m_instrumentName == "D16") {
    wavelength = entry.getFloat(instrumentNamePath + "/Beam/wavelength");
  } else {
    wavelength = entry.getFloat(instrumentNamePath + "/selector/wavelength");
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
    runDetails.addProperty<double>("wavelength", wavelength);
    double ei = m_loadHelper.calculateEnergy(wavelength);
    runDetails.addProperty<double>("Ei", ei, true);
    // wavelength
  }
  // Add a log called timer with the value of duration
  const double duration = entry.getFloat("duration");
  runDetails.addProperty<double>("timer", duration);

  // the start time is needed in the workspace when loading the parameter file
  std::string startDate = entry.getString("start_time");
  runDetails.addProperty<std::string>("start_time", m_loadHelper.dateTimeInIsoFormat(startDate));
  // set the facility
  runDetails.addProperty<std::string>("Facility", std::string("ILL"));
}

/**
 * Sets full sample logs
 * @param filename : name of the file
 */
void LoadILLSANS2::setFinalProperties(const std::string &filename) {
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty("is_frame_skipping", 0);
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(filename.c_str(), NXACC_READ, &nxHandle);

  if (nxStat != NX_ERROR) {
    m_loadHelper.addNexusFieldsToWsRun(nxHandle, runDetails);
    NXclose(&nxHandle);
  }
}

/**
 * Adjusts pixel by pixel the wavelength axis
 * Used only for D33 in TOF mode
 */
void LoadILLSANS2::adjustTOF() {
  const auto &specInfo = m_localWorkspace->spectrumInfo();
  const double l1 = m_sourcePos;
  const size_t nHist = m_localWorkspace->getNumberHistograms();
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_localWorkspace))
  for (int64_t index = 0; index < static_cast<int64_t>(nHist - N_MONITORS); ++index) {
    const double l2 = specInfo.l2(index);
    const double z = specInfo.position(index).Z();
    auto &x = m_localWorkspace->mutableX(index);
    const double scale = (l1 + z) / (l1 + l2);
    std::transform(x.begin(), x.end(), x.begin(), [scale](double lambda) { return scale * lambda; });
  }

  // Try to set sensible (but not strictly physical) wavelength axes for
  // monitors
  // Normalisation is done by acquisition time, so these axes should not be
  // used
  auto firstPixel = m_localWorkspace->histogram(0).dataX();
  const double l2 = specInfo.l2(0);
  const double monitor2 = -specInfo.position(nHist - 1).Z();
  const double l1Monitor2 = m_sourcePos - monitor2;
  const double monScale = (l1 + l2) / l1Monitor2;
  std::transform(firstPixel.begin(), firstPixel.end(), firstPixel.begin(),
                 [monScale](double lambda) { return monScale * lambda; });
  for (size_t mIndex = nHist - N_MONITORS; mIndex < nHist; ++mIndex) {
    const HistogramData::Counts counts = m_localWorkspace->histogram(mIndex).counts();
    const HistogramData::BinEdges binEdges(firstPixel);
    m_localWorkspace->setHistogram(mIndex, std::move(binEdges), std::move(counts));
  }
}

/**
 * Moves the source to the middle of the two master choppers
 * Used only for D33 in TOF mode
 */
void LoadILLSANS2::moveSource() {
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
 * Returns the wavelength axis computed in VTOF mode
 * @param entry : opened root nexus entry
 * @param path : path of the detector distance entry
 * @param sum : loaded channel width sums
 * @param times : loaded channel width times
 * @return binning : wavelength bin boundaries
 */
std::vector<double> LoadILLSANS2::getVariableTimeBinning(const NXEntry &entry, const std::string &path,
                                                         const NXInt &sum, const NXFloat &times) const {
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

/**
 * Figures out the measurement type and sets the default binning relevant to the measurement type.
 * @param firstEntry : opened root nexus entry, used to obtain data dimensions
 * @return binning : wavelength bin boundaries
 */
std::tuple<int, int, int> LoadILLSANS2::getDataDimensions(NeXus::NXEntry &firstEntry) {
  std::tuple<int, int, int> dataDimensions;
  std::vector<std::string> defaultInstruments{"D11", "D16", "D22"};
  if (std::find(defaultInstruments.begin(), defaultInstruments.end(), m_instrumentName) != defaultInstruments.end()) {
    std::string path = "data";
    if (!firstEntry.containsGroup("data")) {
      path = "data_scan/detector_data/data";
    }
    NXData dataGroup = firstEntry.openNXData(path);
    NXInt data = dataGroup.openIntData();
    dataDimensions = std::make_tuple(data.dim0(), data.dim1(), data.dim2());
  } else if (m_instrumentName == "D11B") {
    NXData dataDet1 = firstEntry.openNXData("D11/Detector 1/data");
    NXInt data = dataDet1.openIntData();
    dataDimensions = std::make_tuple(data.dim0(), data.dim1(), data.dim2());
  } else if (m_instrumentName == "D22B") {
    NXData dataDet1 = firstEntry.openNXData("data1");
    NXInt data = dataDet1.openIntData();
    dataDimensions = std::make_tuple(data.dim0(), data.dim1(), data.dim2());
  }

  return dataDimensions;
}

/**
 * Figures out the measurement type, sets omega scan flag for D16 data, and prepares a default binning relevant to the
 * measurement type.
 * @param entry : opened root nexus entry, used to obtain data dimensions
 * @return binning : wavelength bin boundaries
 */
void LoadILLSANS2::figureOutMeasurementType(NeXus::NXEntry &entry) {
  m_measurementType = MeasurementType::MONO;
  auto dataDimensions = getDataDimensions(entry);
  if (m_instrumentName == "D33" && entry.getFloat("mode") == 1.0) { // TOF
    m_measurementType = MeasurementType::TOF;
  } else {
    m_isD16Omega = (m_instrumentName == "D16" && std::get<0>(dataDimensions) == 1 && std::get<2>(dataDimensions) > 1);
    if (std::get<2>(dataDimensions) > 1 && !m_isD16Omega)
      m_measurementType = MeasurementType::KINETIC;
  }

  // in non-monochromatic case, the binning will contain proper indices according to the size of data
  if (m_measurementType != MeasurementType::MONO) {
    // TOF mode is a histogram data, thus it needs one extra bin edge
    auto isTOF = m_measurementType == MeasurementType::TOF ? 1 : 0;
    std::vector<double> frames(std::get<2>(dataDimensions) + isTOF, 0);
    for (int i = 0; i < std::get<2>(dataDimensions) + isTOF; ++i) {
      frames[i] = i;
    }
    m_defaultBinning.resize(std::get<2>(dataDimensions) + isTOF);
    std::copy(frames.cbegin(), frames.cend(), m_defaultBinning.begin());
  }

  if (m_instrumentName == "D22B" && !m_loadInstrument) {
    m_loadInstrument = true;
    g_log.information("LoadInstrument property not chosen but due to detector index swap in cycle 211, this option is "
                      "not available for D22B."
                      " The instrument will be loaded.");
  }
}

} // namespace DataHandling
} // namespace Mantid
