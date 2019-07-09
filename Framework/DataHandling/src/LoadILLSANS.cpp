// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLSANS.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/Path.h>
#include <cmath>
#include <limits>
#include <numeric> // std::accumulate

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
    : m_supportedInstruments{"D11", "D22", "D33"}, m_defaultBinning{0, 0},
      m_resMode("nominal"), m_isTOF(false), m_sourcePos(0.) {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLSANS::name() const { return "LoadILLSANS"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLSANS::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLSANS::category() const {
  return "DataHandling\\Nexus;ILL\\SANS";
}

/// Algorithm's summary. @see Algorithm::summery
const std::string LoadILLSANS::summary() const {
  return "Loads ILL nexus files for SANS instruments D11, D22, D33.";
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
  if (descriptor.pathExists("/entry0/reactor_power") &&
      descriptor.pathExists("/entry0/instrument_name") &&
      descriptor.pathExists("/entry0/mode")) {
    return 80;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLSANS::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "",
                                                 FileProperty::Load, ".nxs"),
                  "Name of the nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLSANS::exec() {
  const std::string filename = getPropertyValue("Filename");
  NXRoot root(filename);
  NXEntry firstEntry = root.openFirstEntry();
  const std::string instrumentPath =
      m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);
  Progress progress(this, 0.0, 1.0, 4);

  if (m_instrumentName == "D33") {
    progress.report("Initializing the workspace for " + m_instrumentName);
    initWorkSpaceD33(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();
    const DetectorPosition detPos =
        getDetectorPositionD33(firstEntry, instrumentPath);
    progress.report("Moving detectors");
    moveDetectorsD33(std::move(detPos));
    if (m_isTOF) {
      adjustTOF();
      moveSource();
    }
  } else {
    progress.report("Initializing the workspace for " + m_instrumentName);
    initWorkSpace(firstEntry, instrumentPath);
    progress.report("Loading the instrument " + m_instrumentName);
    runLoadInstrument();
    double distance = m_loader.getDoubleFromNexusPath(
        firstEntry, instrumentPath + "/detector/det_calc");
    progress.report("Moving detectors");
    moveDetectorDistance(distance, "detector");
    if (m_instrumentName == "D22") {
      double offset = m_loader.getDoubleFromNexusPath(
          firstEntry, instrumentPath + "/detector/dtr_actual");
      moveDetectorHorizontal(offset / 1000, "detector"); // mm to meter
      /*TODO: DO NOT ROTATE UNTIL CONFIRMED BY INSTRUMENT SCIENTIST
      double angle = m_loader.getDoubleFromNexusPath(
          firstEntry, instrumentPath + "/detector/dan_actual");
      rotateD22(angle, "detector");*/
    }
  }

  progress.report("Setting sample logs");
  setFinalProperties(filename);
  setPixelSize();
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Set member variable with the instrument name
 */
void LoadILLSANS::setInstrumentName(const NeXus::NXEntry &firstEntry,
                                    const std::string &instrumentNamePath) {
  if (instrumentNamePath.empty()) {
    std::string message("Cannot set the instrument name from the Nexus file!");
    g_log.error(message);
    throw std::runtime_error(message);
  }
  m_instrumentName =
      m_loader.getStringFromNexusPath(firstEntry, instrumentNamePath + "/name");
  const auto inst = std::find(m_supportedInstruments.begin(),
                              m_supportedInstruments.end(), m_instrumentName);
  if (inst == m_supportedInstruments.end()) {
    throw std::runtime_error(
        "Instrument " + m_instrumentName +
        " is not supported. Only D11, D22 and D33 are supported");
  }
  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
}

/**
 * Get detector panel distances from the nexus file
 * @return a structure with the positions
 */
LoadILLSANS::DetectorPosition
LoadILLSANS::getDetectorPositionD33(const NeXus::NXEntry &firstEntry,
                                    const std::string &instrumentNamePath) {
  std::string detectorPath(instrumentNamePath + "/detector");
  DetectorPosition pos;
  pos.distanceSampleRear =
      m_loader.getDoubleFromNexusPath(firstEntry, detectorPath + "/det2_calc");
  pos.distanceSampleBottomTop =
      m_loader.getDoubleFromNexusPath(firstEntry, detectorPath + "/det1_calc");
  pos.distanceSampleRightLeft =
      pos.distanceSampleBottomTop +
      m_loader.getDoubleFromNexusPath(firstEntry,
                                      detectorPath + "/det1_panel_separation");
  pos.shiftLeft = m_loader.getDoubleFromNexusPath(
                      firstEntry, detectorPath + "/OxL_actual") *
                  1e-3;
  pos.shiftRight = m_loader.getDoubleFromNexusPath(
                       firstEntry, detectorPath + "/OxR_actual") *
                   1e-3;
  pos.shiftUp = m_loader.getDoubleFromNexusPath(firstEntry,
                                                detectorPath + "/OyT_actual") *
                1e-3;
  pos.shiftDown = m_loader.getDoubleFromNexusPath(
                      firstEntry, detectorPath + "/OyB_actual") *
                  1e-3;
  pos >> g_log.debug();
  return pos;
}

/**
 * Loads data for D11 and D22
 */
void LoadILLSANS::initWorkSpace(NeXus::NXEntry &firstEntry,
                                const std::string &instrumentPath) {
  g_log.debug("Fetching data...");
  NXData dataGroup = firstEntry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  data.load();
  const size_t numberOfHistograms =
      static_cast<size_t>(data.dim0() * data.dim1()) + N_MONITORS;
  createEmptyWorkspace(numberOfHistograms, 1);
  loadMetaData(firstEntry, instrumentPath);
  size_t nextIndex =
      loadDataIntoWorkspaceFromVerticalTubes(data, m_defaultBinning, 0);
  nextIndex = loadDataIntoWorkspaceFromMonitors(firstEntry, nextIndex);
  if (data.dim1() == 128) {
    m_resMode = "low";
  }
}

/**
 * Loads data for D33
 */
void LoadILLSANS::initWorkSpaceD33(NeXus::NXEntry &firstEntry,
                                   const std::string &instrumentPath) {

  g_log.debug("Fetching data...");

  NXData dataGroup1 = firstEntry.openNXData("data1");
  NXInt dataRear = dataGroup1.openIntData();
  dataRear.load();
  NXData dataGroup2 = firstEntry.openNXData("data2");
  NXInt dataLeft = dataGroup2.openIntData();
  dataLeft.load();
  NXData dataGroup3 = firstEntry.openNXData("data3");
  NXInt dataRight = dataGroup3.openIntData();
  dataRight.load();
  NXData dataGroup4 = firstEntry.openNXData("data4");
  NXInt dataDown = dataGroup4.openIntData();
  dataDown.load();
  NXData dataGroup5 = firstEntry.openNXData("data5");
  NXInt dataUp = dataGroup5.openIntData();
  dataUp.load();
  g_log.debug("Checking channel numbers...");

  // check number of channels
  if (dataRear.dim2() != dataRight.dim2() &&
      dataRight.dim2() != dataLeft.dim2() &&
      dataLeft.dim2() != dataDown.dim2() && dataDown.dim2() != dataUp.dim2()) {
    throw std::runtime_error(
        "The time bins have not the same dimension for all the 5 detectors!");
  }
  const size_t numberOfHistograms = static_cast<size_t>(
      dataRear.dim0() * dataRear.dim1() + dataRight.dim0() * dataRight.dim1() +
      dataLeft.dim0() * dataLeft.dim1() + dataDown.dim0() * dataDown.dim1() +
      dataUp.dim0() * dataUp.dim1());

  g_log.debug("Creating empty workspace...");
  createEmptyWorkspace(numberOfHistograms + N_MONITORS,
                       static_cast<size_t>(dataRear.dim2()));

  loadMetaData(firstEntry, instrumentPath);

  std::vector<double> binningRear, binningRight, binningLeft, binningDown,
      binningUp;

  if (firstEntry.getFloat("mode") == 0.0) { // Not TOF
    g_log.debug("Getting default wavelength bins...");
    binningRear = m_defaultBinning;
    binningRight = m_defaultBinning;
    binningLeft = m_defaultBinning;
    binningDown = m_defaultBinning;
    binningUp = m_defaultBinning;

  } else { // TOF
    m_isTOF = true;
    NXInt masterPair =
        firstEntry.openNXInt(m_instrumentName + "/tof/master_pair");
    masterPair.load();

    const std::string first = std::to_string(masterPair[0]);
    const std::string second = std::to_string(masterPair[1]);
    g_log.debug("Master choppers are " + first + " and " + second);

    NXFloat firstChopper = firstEntry.openNXFloat(
        m_instrumentName + "/chopper" + first + "/sample_distance");
    firstChopper.load();
    NXFloat secondChopper = firstEntry.openNXFloat(
        m_instrumentName + "/chopper" + second + "/sample_distance");
    secondChopper.load();
    m_sourcePos = (firstChopper[0] + secondChopper[0]) / 2.;
    g_log.debug("Source distance computed, moving moderator to Z=-" +
                std::to_string(m_sourcePos));
    g_log.debug("Getting wavelength bins from the nexus file...");
    bool vtof = true;
    // try VTOF mode
    try {
      NXInt channelWidthSum =
          firstEntry.openNXInt(m_instrumentName + "/tof/chwidth_sum");
      NXFloat channelWidthTimes =
          firstEntry.openNXFloat(m_instrumentName + "/tof/chwidth_times");
      channelWidthSum.load();
      channelWidthTimes.load();
      std::string distancePrefix(instrumentPath + "/tof/tof_distance_detector");
      binningRear = getVariableTimeBinning(firstEntry, distancePrefix + "1",
                                           channelWidthSum, channelWidthTimes);
      binningLeft = getVariableTimeBinning(firstEntry, distancePrefix + "2",
                                           channelWidthSum, channelWidthTimes);
      binningRight = getVariableTimeBinning(firstEntry, distancePrefix + "3",
                                            channelWidthSum, channelWidthTimes);
      binningDown = getVariableTimeBinning(firstEntry, distancePrefix + "4",
                                           channelWidthSum, channelWidthTimes);
      binningUp = getVariableTimeBinning(firstEntry, distancePrefix + "5",
                                         channelWidthSum, channelWidthTimes);
    } catch (const std::runtime_error &) {
      vtof = false;
    }
    if (!vtof) {
      try {
        // LTOF mode
        std::string binPathPrefix(instrumentPath +
                                  "/tof/tof_wavelength_detector");
        binningRear = m_loader.getTimeBinningFromNexusPath(firstEntry,
                                                           binPathPrefix + "1");

        binningLeft = m_loader.getTimeBinningFromNexusPath(firstEntry,
                                                           binPathPrefix + "2");
        binningRight = m_loader.getTimeBinningFromNexusPath(
            firstEntry, binPathPrefix + "3");
        binningDown = m_loader.getTimeBinningFromNexusPath(firstEntry,
                                                           binPathPrefix + "4");
        binningUp = m_loader.getTimeBinningFromNexusPath(firstEntry,
                                                         binPathPrefix + "5");
      } catch (std::runtime_error &e) {
        throw std::runtime_error(
            "Unable to load the wavelength axes for TOF data " +
            std::string(e.what()));
      }
    }
  }

  g_log.debug("Loading the data into the workspace...");

  size_t nextIndex =
      loadDataIntoWorkspaceFromVerticalTubes(dataRear, binningRear, 0);
  nextIndex = loadDataIntoWorkspaceFromVerticalTubes(dataRight, binningRight,
                                                     nextIndex);
  nextIndex =
      loadDataIntoWorkspaceFromVerticalTubes(dataLeft, binningLeft, nextIndex);
  nextIndex =
      loadDataIntoWorkspaceFromVerticalTubes(dataDown, binningDown, nextIndex);
  nextIndex =
      loadDataIntoWorkspaceFromVerticalTubes(dataUp, binningUp, nextIndex);
  nextIndex = loadDataIntoWorkspaceFromMonitors(firstEntry, nextIndex);
}

size_t
LoadILLSANS::loadDataIntoWorkspaceFromMonitors(NeXus::NXEntry &firstEntry,
                                               size_t firstIndex) {

  // let's find the monitors; should be monitor1 and monitor2
  for (std::vector<NXClassInfo>::const_iterator it =
           firstEntry.groups().begin();
       it != firstEntry.groups().end(); ++it) {
    if (it->nxclass == "NXmonitor") {
      NXData dataGroup = firstEntry.openNXData(it->nxname);
      NXInt data = dataGroup.openIntData();
      data.load();
      g_log.debug() << "Monitor: " << it->nxname << " dims = " << data.dim0()
                    << "x" << data.dim1() << "x" << data.dim2() << '\n';
      const size_t vectorSize = data.dim2() + 1;
      HistogramData::BinEdges histoBinEdges(
          vectorSize, HistogramData::LinearGenerator(0.0, 1));
      if (!m_isTOF) { // Not TOF
        histoBinEdges = HistogramData::BinEdges(m_defaultBinning);
      }
      const HistogramData::Counts histoCounts(data(), data() + data.dim2());
      m_localWorkspace->setHistogram(firstIndex, std::move(histoBinEdges),
                                     std::move(histoCounts));
      // Add average monitor counts to a property:
      double averageMonitorCounts =
          std::accumulate(data(), data() + data.dim2(), 0) /
          static_cast<double>(data.dim2());
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

size_t LoadILLSANS::loadDataIntoWorkspaceFromVerticalTubes(
    NeXus::NXInt &data, const std::vector<double> &timeBinning,
    size_t firstIndex = 0) {

  // Workaround to get the number of tubes / pixels
  const size_t numberOfTubes = data.dim0();
  const size_t numberOfPixelsPerTube = data.dim1();
  const HistogramData::BinEdges binEdges(timeBinning);

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_localWorkspace))
  for (int i = 0; i < static_cast<int>(numberOfTubes); ++i) {
    for (size_t j = 0; j < numberOfPixelsPerTube; ++j) {
      const int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      const size_t index = firstIndex + i * numberOfPixelsPerTube + j;
      const HistogramData::Counts histoCounts(data_p, data_p + data.dim2());
      m_localWorkspace->setHistogram(index, binEdges, std::move(histoCounts));
    }
  }

  return firstIndex + numberOfTubes * numberOfPixelsPerTube;
}

/**
 * Create a workspace without any data in it
 * @param numberOfHistograms : number of spectra
 * @param numberOfChannels : number of TOF channels
 */
void LoadILLSANS::createEmptyWorkspace(const size_t numberOfHistograms,
                                       const size_t numberOfChannels) {
  m_localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", numberOfHistograms, numberOfChannels + 1,
      numberOfChannels);
  m_localWorkspace->getAxis(0)->unit() =
      UnitFactory::Instance().create("Wavelength");
  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Makes up the full path of the relevant IDF dependent on resolution mode
 * @param instName : the name of the instrument (including the resolution mode
 * suffix)
 * @return : the full path to the corresponding IDF
 */
std::string
LoadILLSANS::getInstrumentFilePath(const std::string &instName) const {

  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(instName + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

/**
 * Loads the instrument from the IDF
 */
void LoadILLSANS::runLoadInstrument() {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  if (m_resMode == "nominal") {
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
  } else if (m_resMode == "low") {
    loadInst->setPropertyValue("Filename",
                               getInstrumentFilePath(m_instrumentName + "lr"));
  }
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  loadInst->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(true));
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
void LoadILLSANS::moveDetectorDistance(double distance,
                                       const std::string &componentName) {

  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);
  mover->setProperty("X", pos.X());
  mover->setProperty("Y", pos.Y());
  mover->setProperty("Z", distance);
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();
  g_log.debug() << "Moving component '" << componentName
                << "' to Z = " << distance << '\n';
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty<double>("L2", distance, true);
}

/**
 * Rotates D22 detector around y-axis
 * @param angle : the angle to rotate [degree]
 * @param componentName : "detector"
 */
void LoadILLSANS::rotateD22(double angle, const std::string &componentName) {
  API::IAlgorithm_sptr rotater =
      createChildAlgorithm("RotateInstrumentComponent");
  rotater->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  rotater->setProperty("ComponentName", componentName);
  rotater->setProperty("X", 0.);
  rotater->setProperty("Y", 1.);
  rotater->setProperty("Z", 0.);
  rotater->setProperty("Angle", angle);
  rotater->setProperty("RelativeRotation", false);
  rotater->executeAsChildAlg();
  g_log.debug() << "Rotating component '" << componentName
                << "' to angle = " << angle << " degrees.\n";
}

/**
 * Move detectors in X
 * @param shift : the distance to move [metres]
 * @param componentName : the name of the component
 */
void LoadILLSANS::moveDetectorHorizontal(double shift,
                                         const std::string &componentName) {
  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);
  mover->setProperty("X", shift);
  mover->setProperty("Y", pos.Y());
  mover->setProperty("Z", pos.Z());
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();
  g_log.debug() << "Moving component '" << componentName << "' to X = " << shift
                << '\n';
}

/**
 * Move detectors in Y
 * @param shift : the distance to move [metres]
 * @param componentName : the name of the component
 */
void LoadILLSANS::moveDetectorVertical(double shift,
                                       const std::string &componentName) {
  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", componentName);
  mover->setProperty("X", pos.X());
  mover->setProperty("Y", shift);
  mover->setProperty("Z", pos.Z());
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();
  g_log.debug() << "Moving component '" << componentName << "' to Y = " << shift
                << '\n';
}

/**
 * Get position of a component
 * @param componentName : the name of the component
 * @return : V3D of the component position
 */
V3D LoadILLSANS::getComponentPosition(const std::string &componentName) {
  Geometry::Instrument_const_sptr instrument =
      m_localWorkspace->getInstrument();
  Geometry::IComponent_const_sptr component =
      instrument->getComponentByName(componentName);
  return component->getPos();
}

/**
 * Loads some metadata present in the nexus file
 * @param entry : opened nexus entry
 * @param instrumentNamePath : the nexus entry of the instrument
 */
void LoadILLSANS::loadMetaData(const NeXus::NXEntry &entry,
                               const std::string &instrumentNamePath) {

  g_log.debug("Loading metadata...");
  API::Run &runDetails = m_localWorkspace->mutableRun();

  if (entry.getFloat("mode") == 0.0) { // Not TOF
    runDetails.addProperty<std::string>("tof_mode", "Non TOF");
  } else {
    runDetails.addProperty<std::string>("tof_mode", "TOF");
  }

  double wavelength =
      entry.getFloat(instrumentNamePath + "/selector/wavelength");
  g_log.debug() << "Wavelength found in the nexus file: " << wavelength << '\n';
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
        g_log.warning("Could not find wavelength resolution, assuming 10%");
      }
    }
    runDetails.addProperty<double>("wavelength", wavelength);
    double ei = m_loader.calculateEnergy(wavelength);
    runDetails.addProperty<double>("Ei", ei, true);
    // wavelength
    m_defaultBinning[0] = wavelength - wavelengthRes * wavelength * 0.01 / 2;
    m_defaultBinning[1] = wavelength + wavelengthRes * wavelength * 0.01 / 2;
  }
  // Add a log called timer with the value of duration
  const double duration = entry.getFloat("duration");
  runDetails.addProperty<double>("timer", duration);
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
    m_loader.addNexusFieldsToWsRun(nxHandle, runDetails);
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
  for (int64_t index = 0; index < static_cast<int64_t>(nHist - N_MONITORS);
       ++index) {
    const double l2 = specInfo.l2(index);
    const double z = specInfo.position(index).Z();
    auto &x = m_localWorkspace->mutableX(index);
    const double scale = (l1 + z) / (l1 + l2);
    std::transform(x.begin(), x.end(), x.begin(),
                   [scale](double lambda) { return scale * lambda; });
  }

  // Try to set sensible (but not strictly physical) wavelength axes for
  // monitors
  // Normalisation is done by acquisition time, so these axes should not be used
  auto firstPixel = m_localWorkspace->histogram(0).dataX();
  const double l2 = specInfo.l2(0);
  const double monitor2 = -specInfo.position(nHist - 1).Z();
  const double l1Monitor2 = m_sourcePos - monitor2;
  const double monScale = (l1 + l2) / l1Monitor2;
  std::transform(firstPixel.begin(), firstPixel.end(), firstPixel.begin(),
                 [monScale](double lambda) { return monScale * lambda; });
  for (size_t mIndex = nHist - N_MONITORS; mIndex < nHist; ++mIndex) {
    const HistogramData::Counts counts =
        m_localWorkspace->histogram(mIndex).counts();
    const HistogramData::BinEdges binEdges(firstPixel);
    m_localWorkspace->setHistogram(mIndex, std::move(binEdges),
                                   std::move(counts));
  }
}

/**
 * Moves the source to the middle of the two master choppers
 * Used only for D33 in TOF mode
 */
void LoadILLSANS::moveSource() {
  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");
  mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mover->setProperty("ComponentName", "moderator");
  mover->setProperty("X", 0.);
  mover->setProperty("Y", 0.);
  mover->setProperty("Z", -m_sourcePos);
  mover->setProperty("RelativePosition", false);
  mover->executeAsChildAlg();
}

/**
 * Sets the width (x) and height (y) of the pixel
 */
void LoadILLSANS::setPixelSize() {
  const auto instrument = m_localWorkspace->getInstrument();
  const std::string component =
      (m_instrumentName == "D33") ? "back_detector" : "detector";
  auto detector = instrument->getComponentByName(component);
  auto rectangle =
      boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(
          detector);
  if (rectangle) {
    const double dx = rectangle->xstep();
    const double dy = rectangle->ystep();
    API::Run &runDetails = m_localWorkspace->mutableRun();
    runDetails.addProperty<double>("pixel_width", dx);
    runDetails.addProperty<double>("pixel_height", dy);
  } else {
    g_log.debug("No pixel size available");
  }
}

/**
 * Returns the wavelength axis computed in VTOF mode
 * @param entry : opened root nexus entry
 * @param path : path of the detector distance entry
 * @param sum : loaded channel width sums
 * @param times : loaded channel width times
 * @return binning : wavelength bin boundaries
 */
std::vector<double>
LoadILLSANS::getVariableTimeBinning(const NXEntry &entry,
                                    const std::string &path, const NXInt &sum,
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
    const double lambda = PhysicalConstants::h /
                          PhysicalConstants::NeutronMass / velocity * 1E+10;
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
