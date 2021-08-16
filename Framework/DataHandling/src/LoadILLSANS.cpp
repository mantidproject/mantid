// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLSANS.h"
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

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLSANS)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLSANS::LoadILLSANS()
    : m_supportedInstruments{"D11", "D22", "D33", "D16"}, m_defaultBinning{0, 0}, m_resMode("nominal"), m_isTOF(false),
      m_sourcePos(0.) {}

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
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLSANS::exec() {
  const std::string filename = getPropertyValue("Filename");
  m_isD16Omega = false;
  NXRoot root(filename);
  NXEntry firstEntry = root.openFirstEntry();
  const std::string instrumentPath = m_loadHelper.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);
  Progress progress(this, 0.0, 1.0, 4);
  progress.report("Initializing the workspace for " + m_instrumentName);
  if (m_instrumentName == "D33") {
    initWorkSpaceD33(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();
    const DetectorPosition detPos = getDetectorPositionD33(firstEntry, instrumentPath);
    progress.report("Moving detectors");
    moveDetectorsD33(std::move(detPos));
    if (m_isTOF) {
      adjustTOF();
      moveSource();
    }

  } else if (m_instrumentName == "D16") {
    initWorkSpace(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();

    double distance = firstEntry.getFloat(instrumentPath + "/Det/value") / 1000; // mm to metre
    const double angle = firstEntry.getFloat(instrumentPath + "/Gamma/value");
    placeD16(-angle, distance, "detector");

  } else if (m_instrumentName == "D11B") {
    initWorkSpaceD11B(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();

    // we move the parent "detector" component, but since it is at (0,0,0), we
    // need to find the distance it has to move and move it to this position
    double finalDistance = firstEntry.getFloat(instrumentPath + "/Detector 1/det_calc");
    V3D pos = getComponentPosition("detector_center");
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
    double distance = m_loadHelper.getDoubleFromNexusPath(firstEntry, instrumentPath + "/detector/det_calc");
    progress.report("Moving detectors");
    moveDetectorDistance(distance, "detector");
    API::Run &runDetails = m_localWorkspace->mutableRun();
    runDetails.addProperty<double>("L2", distance, true);
    if (m_instrumentName == "D22") {
      double offset = m_loadHelper.getDoubleFromNexusPath(firstEntry, instrumentPath + "/detector/dtr_actual");
      moveDetectorHorizontal(-offset / 1000, "detector"); // mm to meter
    }
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
LoadILLSANS::DetectorPosition LoadILLSANS::getDetectorPositionD33(const NeXus::NXEntry &firstEntry,
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
  pos >> g_log.debug();
  return pos;
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
  NXData dataGroup = firstEntry.openNXData(path);
  NXInt data = dataGroup.openIntData();
  data.load();
  size_t numberOfHistograms;

  m_isD16Omega = (data.dim0() == 1 && data.dim2() > 1 && m_instrumentName == "D16");

  if (m_isD16Omega) {
    numberOfHistograms = static_cast<size_t>(data.dim1() * data.dim2()) + N_MONITORS;
  } else {
    numberOfHistograms = static_cast<size_t>(data.dim0() * data.dim1()) + N_MONITORS;
  }
  createEmptyWorkspace(numberOfHistograms, 1);
  loadMetaData(firstEntry, instrumentPath);

  size_t nextIndex;
  nextIndex = loadDataFromTubes(data, m_defaultBinning, 0);
  nextIndex = loadDataFromMonitors(firstEntry, nextIndex);
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

  if (dataCenter.dim2() != 1) {
    createEmptyWorkspace(numberOfHistograms, dataCenter.dim2(), MultichannelType::KINETIC);
  } else {
    createEmptyWorkspace(numberOfHistograms, 1);
  }
  loadMetaData(firstEntry, instrumentPath);

  // we need to adjust the default binning after loadmetadata
  if (dataCenter.dim2() != 1) {
    std::vector<double> frames(dataCenter.dim2(), 0);
    for (int i = 0; i < dataCenter.dim2(); ++i) {
      frames[i] = i;
    }
    m_defaultBinning.resize(dataCenter.dim2());
    std::copy(frames.cbegin(), frames.cend(), m_defaultBinning.begin());
  }

  MultichannelType type = (dataCenter.dim2() != 1) ? MultichannelType::KINETIC : MultichannelType::TOF;
  size_t nextIndex;
  nextIndex = loadDataFromTubes(dataCenter, m_defaultBinning, 0, type);
  nextIndex = loadDataFromTubes(dataLeft, m_defaultBinning, nextIndex, type);
  nextIndex = loadDataFromTubes(dataRight, m_defaultBinning, nextIndex, type);
  nextIndex = loadDataFromMonitors(firstEntry, nextIndex, type);
  if (dataCenter.dim2() != 1 && nextIndex < numberOfHistograms) {
    // there are a few runs with no 2nd monitor in kinetic, so we load the first monitor once again to preserve the
    // dimensions and x-axis
    nextIndex = loadDataFromMonitors(firstEntry, nextIndex, type);
  }
  // hijack the second monitor spectrum to store per-frame durations to enable time normalisation
  if (dataCenter.dim2() != 1) {
    NXFloat durations = firstEntry.openNXFloat("slices");
    durations.load();
    const HistogramData::Counts histoCounts(durations(), durations() + dataCenter.dim2());
    m_localWorkspace->setCounts(nextIndex - 1, std::move(histoCounts));
    m_localWorkspace->setCountVariances(nextIndex - 1,
                                        HistogramData::CountVariances(std::vector<double>(dataCenter.dim2(), 0)));
  }
}

/**
 * @brief LoadILLSANS::initWorkSpaceD22B Load D22B data
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS::initWorkSpaceD22B(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {
  g_log.debug("Fetching data...");

  NXData data2 = firstEntry.openNXData("data2");
  NXInt data2_data = data2.openIntData();
  data2_data.load();
  NXData data1 = firstEntry.openNXData("data1");
  NXInt data1_data = data1.openIntData();
  data1_data.load();

  size_t numberOfHistograms =
      static_cast<size_t>(data2_data.dim0() * data2_data.dim1() + data1_data.dim0() * data1_data.dim1()) + N_MONITORS;

  createEmptyWorkspace(numberOfHistograms, 1);
  loadMetaData(firstEntry, instrumentPath);
  runLoadInstrument();

  const std::string backIndex = m_localWorkspace->getInstrument()->getStringParameter("back_detector_index")[0];
  size_t nextIndex;
  if (backIndex == "2") {
    nextIndex = loadDataFromTubes(data2_data, m_defaultBinning, 0);
    nextIndex = loadDataFromTubes(data1_data, m_defaultBinning, nextIndex);
  } else {
    nextIndex = loadDataFromTubes(data1_data, m_defaultBinning, 0);
    nextIndex = loadDataFromTubes(data2_data, m_defaultBinning, nextIndex);
  }
  nextIndex = loadDataFromMonitors(firstEntry, nextIndex);
}

/**
 * Loads data for D33
 * @param firstEntry : already opened first entry in nexus
 * @param instrumentPath : the path inside nexus where the instrument name is written
 */
void LoadILLSANS::initWorkSpaceD33(NeXus::NXEntry &firstEntry, const std::string &instrumentPath) {

  g_log.debug("Fetching data...");

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
  createEmptyWorkspace(numberOfHistograms + N_MONITORS, static_cast<size_t>(dataRear.dim2()));

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

  g_log.debug("Loading the data into the workspace...");

  size_t nextIndex = loadDataFromTubes(dataRear, binningRear, 0);
  nextIndex = loadDataFromTubes(dataRight, binningRight, nextIndex);
  nextIndex = loadDataFromTubes(dataLeft, binningLeft, nextIndex);
  nextIndex = loadDataFromTubes(dataDown, binningDown, nextIndex);
  nextIndex = loadDataFromTubes(dataUp, binningUp, nextIndex);
  nextIndex = loadDataFromMonitors(firstEntry, nextIndex);
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
      NXData dataGroup = firstEntry.openNXData(it->nxname);
      NXInt data = dataGroup.openIntData();
      data.load();
      g_log.debug() << "Monitor: " << it->nxname << " dims = " << data.dim0() << "x" << data.dim1() << "x"
                    << data.dim2() << '\n';
      const HistogramData::Counts histoCounts(data(), data() + data.dim2());
      m_localWorkspace->setCounts(firstIndex, std::move(histoCounts));
      const HistogramData::CountVariances histoVariances(data(), data() + data.dim2());
      m_localWorkspace->setCountVariances(firstIndex, std::move(histoVariances));

      if (m_isTOF) {
        HistogramData::BinEdges histoBinEdges(data.dim2() + 1, HistogramData::LinearGenerator(0.0, 1));
        m_localWorkspace->setBinEdges(firstIndex, std::move(histoBinEdges));
      } else {
        if (m_isD16Omega) {
          HistogramData::Points histoPoints =
              HistogramData::Points(std::vector<double>(1, 0.5 * (m_defaultBinning[0] + m_defaultBinning[1])));
          m_localWorkspace->setPoints(firstIndex, std::move(histoPoints));
        } else {
          if (type != MultichannelType::KINETIC) {
            HistogramData::BinEdges histoBinEdges = HistogramData::BinEdges(m_defaultBinning);
            m_localWorkspace->setBinEdges(firstIndex, std::move(histoBinEdges));
          } else {
            HistogramData::Points histoPoints = HistogramData::Points(m_defaultBinning);
            m_localWorkspace->setPoints(firstIndex, std::move(histoPoints));
          }
        }
      }
      // Add average monitor counts to a property:
      double averageMonitorCounts = std::accumulate(data(), data() + data.dim2(), 0) / static_cast<double>(data.dim2());
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
 * @brief Loads data from tubes
 * @param data : a reference to already loaded nexus data block
 * @param timeBinning : the x-axis binning
 * @param firstIndex : the workspace index to start loading to
 * @param type : used to discrimante between TOF and Kinetic
 * @return the next ws index after all the tubes in the given detector bank
 */
size_t LoadILLSANS::loadDataFromTubes(NeXus::NXInt &data, const std::vector<double> &timeBinning, size_t firstIndex = 0,
                                      const MultichannelType type) {
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

      if (m_isD16Omega) {
        const HistogramData::Points histoPoints(std::vector<double>(1, 0.5 * (timeBinning[0] + timeBinning[1])));
        m_localWorkspace->setPoints(index, std::move(histoPoints));
      } else {
        if (type == MultichannelType::KINETIC) {
          const HistogramData::Points histoPoints(timeBinning);
          m_localWorkspace->setPoints(index, std::move(histoPoints));
        } else {
          const HistogramData::BinEdges binEdges(timeBinning);
          m_localWorkspace->setBinEdges(index, std::move(binEdges));
        }
      }
    }
  }

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
  if (type == MultichannelType::TOF) {
    m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
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
  V3D pos = getComponentPosition(componentName);
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
void LoadILLSANS::moveDetectorVertical(double shift, const std::string &componentName) {
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
V3D LoadILLSANS::getComponentPosition(const std::string &componentName) {
  Geometry::Instrument_const_sptr instrument = m_localWorkspace->getInstrument();
  Geometry::IComponent_const_sptr component = instrument->getComponentByName(componentName);
  return component->getPos();
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
    if (m_instrumentName == "D16") {
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
    double ei = m_loadHelper.calculateEnergy(wavelength);
    runDetails.addProperty<double>("Ei", ei, true);
    // wavelength
    m_defaultBinning[0] = wavelength - wavelengthRes * wavelength * 0.01 / 2;
    m_defaultBinning[1] = wavelength + wavelengthRes * wavelength * 0.01 / 2;
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
void LoadILLSANS::setFinalProperties(const std::string &filename) {
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
void LoadILLSANS::adjustTOF() {
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

} // namespace DataHandling
} // namespace Mantid
