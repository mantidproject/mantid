#include "MantidDataHandling/LoadILLSANS.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <limits>
#include <numeric> // std::accumulate
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
    : m_supportedInstruments{"D11", "D22", "D33"}, m_defaultBinning{0, 0},
      m_resMode("nominal"), m_isTOF(false), m_sourcePos(0.) {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLSANS::name() const { return "LoadILLSANS"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLSANS::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLSANS::category() const {
  return "DataHandling\\Nexus;ILL\\SANS;Workflow\\SANS\\UsesPropertyManager";
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
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "Name of the nexus file to load");
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLSANS::exec() {
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName,
                                                        reductionManager);
  }
  // If the load algorithm isn't in the reduction properties, add it
  if (!reductionManager->existsProperty("LoadAlgorithm")) {
    auto algProp = make_unique<AlgorithmProperty>("LoadAlgorithm");
    algProp->setValue(toString());
    reductionManager->declareProperty(std::move(algProp));
  }

  bool moveToBeamCenter = false;
  double center_x = 0., center_y = 0.;
  if (reductionManager->existsProperty("LatestBeamCenterX") &&
      reductionManager->existsProperty("LatestBeamCenterY")) {
    moveToBeamCenter = true;
    center_x = reductionManager->getProperty("LatestBeamCenterX");
    center_y = reductionManager->getProperty("LatestBeamCenterY");
    g_log.debug("Beam center found: X=" + std::to_string(center_x) + "; Y=" +
                std::to_string(center_y));
  } else {
    g_log.debug("No beam center information found in the manager.");
  }

  const std::string filename = getPropertyValue("Filename");
  NXRoot root(filename);
  NXEntry firstEntry = root.openFirstEntry();
  const std::string instrumentPath =
      m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);
  if (!reductionManager->existsProperty("InstrumentName"))
    reductionManager->declareProperty(
        make_unique<PropertyWithValue<std::string>>("InstrumentName",
                                                    m_instrumentName));
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
    setL2SampleLog(detPos.distanceSampleRear);
    if (m_isTOF) {
      adjustTOF();
      moveSource();
    }
    if (moveToBeamCenter) {
        //TODO: move all the panels
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
    setL2SampleLog(distance);
    if (m_instrumentName == "D22") {
      double offset = m_loader.getDoubleFromNexusPath(
          firstEntry, instrumentPath + "/detector/dtr_actual");
      moveDetectorHorizontal(offset / 1000, "detector"); // mm to meter
      /*TODO: DO NOT ROTATE UNTIL CONFIRMED BY INSTRUMENT SCIENTIST
      double angle = m_loader.getDoubleFromNexusPath(
          firstEntry, instrumentPath + "/detector/dan_actual");
      rotateD22(angle, "detector");*/
    }
    if (moveToBeamCenter) {
        moveBeamCenter("detector", center_x, center_y);
    }
  }
  progress.report("Setting sample logs");
  setFinalProperties(filename);
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
  // TODO : Must put this 2 somewhere else: number of monitors!
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
    std::string binPathPrefix(instrumentPath + "/tof/tof_wavelength_detector");

    binningRear =
        m_loader.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "1");
    binningRight =
        m_loader.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "2");
    binningLeft =
        m_loader.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "3");
    binningDown =
        m_loader.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "4");
    binningUp =
        m_loader.getTimeBinningFromNexusPath(firstEntry, binPathPrefix + "5");
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

  g_log.debug("Loading the data into the workspace:");
  g_log.debug() << "\t"
                << "firstIndex = " << firstIndex << '\n';
  g_log.debug() << "\t"
                << "Number of Tubes : data.dim0() = " << data.dim0() << '\n';
  g_log.debug() << "\t"
                << "Number of Pixels : data.dim1() = " << data.dim1() << '\n';
  g_log.debug() << "\t"
                << "data.dim2() = " << data.dim2() << '\n';
  g_log.debug() << "\t"
                << "First bin = " << timeBinning[0] << '\n';

  // Workaround to get the number of tubes / pixels
  const size_t numberOfTubes = data.dim0();
  const size_t numberOfPixelsPerTube = data.dim1();
  const HistogramData::BinEdges binEdges(timeBinning);
  size_t spec = firstIndex;

  for (size_t i = 0; i < numberOfTubes; ++i) {
    for (size_t j = 0; j < numberOfPixelsPerTube; ++j) {
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      const HistogramData::Counts histoCounts(data_p, data_p + data.dim2());
      m_localWorkspace->setHistogram(spec, binEdges, std::move(histoCounts));
      ++spec;
    }
  }

  g_log.debug() << "Data loading inti WS done....\n";

  return spec;
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

  if (wavelength <= 0) {
    g_log.debug() << "Mode = " << entry.getFloat("mode") << '\n';
    g_log.information("The wavelength present in the NeXus file <= 0.");
    if (entry.getFloat("mode") == 0.0) { // Not TOF
      throw std::runtime_error("Working in Non TOF mode and the wavelength in "
                               "the file is <=0 !!! Check with the instrument "
                               "scientist!");
    }
  } else {
    double wavelengthRes =
        entry.getFloat(instrumentNamePath + "/selector/wavelength_res");
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
 * @param lambda : wavelength in Angstroms
 * @param twoTheta : twoTheta in degreess
 * @return Q : momentum transfer [Aˆ-1]
 */
double LoadILLSANS::calculateQ(const double lambda,
                               const double twoTheta) const {
  return (4 * M_PI * std::sin(twoTheta * (M_PI / 180) / 2)) / (lambda);
}

/**
 * Calculates the max and min Q
 * @return pair<min, max>
 */
std::pair<double, double> LoadILLSANS::calculateQMaxQMin() {
  double min = std::numeric_limits<double>::max(),
         max = std::numeric_limits<double>::min();
  g_log.debug("Calculating Qmin Qmax...");
  std::size_t nHist = m_localWorkspace->getNumberHistograms();
  const auto &spectrumInfo = m_localWorkspace->spectrumInfo();
  for (std::size_t i = 0; i < nHist; ++i) {
    if (!spectrumInfo.isMonitor(i)) {
      const auto &lambdaBinning = m_localWorkspace->x(i);
      Kernel::V3D detPos = spectrumInfo.position(i);
      double r, theta, phi;
      detPos.getSpherical(r, theta, phi);
      double v1 = calculateQ(*(lambdaBinning.begin()), theta);
      double v2 = calculateQ(*(lambdaBinning.end() - 1), theta);
      if (i == 0) {
        min = v1;
        max = v1;
      }
      if (v1 < min) {
        min = v1;
      }
      if (v2 < min) {
        min = v2;
      }
      if (v1 > max) {
        max = v1;
      }
      if (v2 > max) {
        max = v2;
      }
    }
  }
  return std::pair<double, double>(min, max);
}

/**
 * Sets full sample logs
 * @param filename : name of the file
 */
void LoadILLSANS::setFinalProperties(const std::string &filename) {
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty("is_frame_skipping", 0);
  std::pair<double, double> minmax = LoadILLSANS::calculateQMaxQMin();
  runDetails.addProperty("qmin", minmax.first);
  runDetails.addProperty("qmax", minmax.second);
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(filename.c_str(), NXACC_READ, &nxHandle);
  if (nxStat != NX_ERROR) {
    m_loader.addNexusFieldsToWsRun(nxHandle, runDetails);
    nxStat = NXclose(&nxHandle);
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
  * Moves the component such that the pixel with center_x, center_y coordinates moves to
  * x=0, y=0 (i.e. z axis).
  * @param center_x : beam center x coord (in pixels)
  * @param center_y : beam center y coord (in pixels)
  */
void LoadILLSANS::moveBeamCenter(const std::string &component, double center_x,
                                 double center_y) {

  const auto &instrument = m_localWorkspace->getInstrument();
  double xPixels = instrument->getNumberParameter("number-of-x-pixels")[0];
  double yPixels = instrument->getNumberParameter("number-of-y-pixels")[0];
  double xPixelSize = instrument->getNumberParameter("x-pixel-size")[0];
  double yPixelSize = instrument->getNumberParameter("y-pixel-size")[0];
  const double xOffset = -(center_x - (xPixels - 1) / 2.) * xPixelSize;
  const double yOffset = -(center_y - (yPixels - 1) / 2.) * yPixelSize;
  IAlgorithm_sptr mvAlg = createChildAlgorithm("MoveInstrumentComponent");
  mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  mvAlg->setProperty("ComponentName", component);
  mvAlg->setProperty("X", xOffset / 1000.);
  mvAlg->setProperty("Y", yOffset / 1000.);
  mvAlg->setProperty("RelativePosition", true);
  mvAlg->executeAsChildAlg();
  g_log.debug() << "Moved beam center with dX=" << xOffset
                << "mm; dY=" << yOffset << "mm.\n";
}

/**
 * Sets a sample log for L2 distance [mm], needed later for reduction
 * Note, for D33 it is the rear detector
 * @param z : the sample-detector-distance in [m]
 */
void LoadILLSANS::setL2SampleLog(const double z) {
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty<double>("sample_detector_distance", z * 1000);
}

} // namespace DataHandling
} // namespace Mantid
