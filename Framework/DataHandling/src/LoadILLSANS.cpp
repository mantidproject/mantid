#include "MantidDataHandling/LoadILLSANS.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <limits>
#include <numeric> // std::accumulate

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLSANS)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLSANS::LoadILLSANS()
    : m_supportedInstruments{"D33"}, m_defaultBinning{0, 0} {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLSANS::name() const { return "LoadILLSANS"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLSANS::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLSANS::category() const {
  return "DataHandling\\Nexus";
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
      "Name of the SPE file to load");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLSANS::exec() {
  // Init
  std::string filename = getPropertyValue("Filename");
  NXRoot root(filename);
  NXEntry firstEntry = root.openFirstEntry();

  std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);

  g_log.debug("Setting detector positions...");
  DetectorPosition detPos = getDetectorPosition(firstEntry, instrumentPath);

  initWorkSpace(firstEntry, instrumentPath);

  // load the instrument from the IDF if it exists
  runLoadInstrument();

  // Move detectors
  moveDetectors(detPos);

  setFinalProperties();
  // Set the output workspace property
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
  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
}

/**
 * Get detector panel distances from the nexus file
 * @return a structure with the positions
 */
DetectorPosition
LoadILLSANS::getDetectorPosition(const NeXus::NXEntry &firstEntry,
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

  g_log.debug() << pos;

  return pos;
}

void LoadILLSANS::initWorkSpace(NeXus::NXEntry &firstEntry,
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
  int numberOfHistograms =
      dataRear.dim0() * dataRear.dim1() + dataRight.dim0() * dataRight.dim1() +
      dataLeft.dim0() * dataLeft.dim1() + dataDown.dim0() * dataDown.dim1() +
      dataUp.dim0() * dataUp.dim1();

  g_log.debug("Creating empty workspace...");
  // TODO : Must put this 2 somewhere else: number of monitors!
  createEmptyWorkspace(numberOfHistograms + 2, dataRear.dim2());

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

  } else {
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
  size_t nextIndex = loadDataIntoWorkspaceFromMonitors(firstEntry, 0);
  nextIndex = loadDataIntoWorkspaceFromHorizontalTubes(dataRear, binningRear,
                                                       nextIndex);
  nextIndex = loadDataIntoWorkspaceFromVerticalTubes(dataRight, binningRight,
                                                     nextIndex);
  nextIndex =
      loadDataIntoWorkspaceFromVerticalTubes(dataLeft, binningLeft, nextIndex);
  nextIndex = loadDataIntoWorkspaceFromHorizontalTubes(dataDown, binningDown,
                                                       nextIndex);
  nextIndex =
      loadDataIntoWorkspaceFromHorizontalTubes(dataUp, binningUp, nextIndex);
}

size_t
LoadILLSANS::loadDataIntoWorkspaceFromMonitors(NeXus::NXEntry &firstEntry,
                                               size_t firstIndex) {

  // let's find the monitors
  // For D33 should be monitor1 and monitor2
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
      std::vector<double> positionsBinning;
      positionsBinning.reserve(vectorSize);

      const HistogramData::BinEdges histoBinEdges(
          vectorSize, HistogramData::LinearGenerator(0.0, 1.0));
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

size_t LoadILLSANS::loadDataIntoWorkspaceFromHorizontalTubes(
    NeXus::NXInt &data, const std::vector<double> &timeBinning,
    size_t firstIndex = 0) {

  g_log.debug("Loading the data into the workspace:");
  g_log.debug() << "\t"
                << "firstIndex = " << firstIndex << '\n';
  g_log.debug() << "\t"
                << "Number of Pixels : data.dim0() = " << data.dim0() << '\n';
  g_log.debug() << "\t"
                << "Number of Tubes : data.dim1() = " << data.dim1() << '\n';
  g_log.debug() << "\t"
                << "data.dim2() = " << data.dim2() << '\n';
  g_log.debug() << "\t"
                << "First bin = " << timeBinning[0] << '\n';

  // Workaround to get the number of tubes / pixels
  const size_t numberOfTubes = data.dim1();
  const size_t numberOfPixelsPerTube = data.dim0();

  Progress progress(this, 0.0, 1.0, data.dim0() * data.dim1());

  size_t spec = firstIndex;

  const HistogramData::BinEdges binEdges(timeBinning);

  for (size_t i = 0; i < numberOfTubes; ++i) {
    for (size_t j = 0; j < numberOfPixelsPerTube; ++j) {
      int *data_p = &data(static_cast<int>(j), static_cast<int>(i), 0);
      const HistogramData::Counts histoCounts(data_p, data_p + data.dim2());
      m_localWorkspace->setHistogram(spec, binEdges, std::move(histoCounts));

      ++spec;
      progress.report();
    }
  }

  g_log.debug() << "Data loading into WS done....\n";

  return spec;
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

  Progress progress(this, 0.0, 1.0, data.dim0() * data.dim1());

  const HistogramData::BinEdges binEdges(timeBinning);
  size_t spec = firstIndex;

  for (size_t i = 0; i < numberOfTubes; ++i) {
    for (size_t j = 0; j < numberOfPixelsPerTube; ++j) {
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      const HistogramData::Counts histoCounts(data_p, data_p + data.dim2());

      m_localWorkspace->setHistogram(spec, binEdges, std::move(histoCounts));
      ++spec;
      progress.report();
    }
  }

  g_log.debug() << "Data loading inti WS done....\n";

  return spec;
}

/***
 * Create a workspace without any data in it
 */
void LoadILLSANS::createEmptyWorkspace(int numberOfHistograms,
                                       int numberOfChannels) {
  m_localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", numberOfHistograms, numberOfChannels + 1,
      numberOfChannels);
  m_localWorkspace->getAxis(0)->unit() =
      UnitFactory::Instance().create("Wavelength");
  m_localWorkspace->setYUnitLabel("Counts");
}

void LoadILLSANS::runLoadInstrument() {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(true));
    loadInst->execute();
  } catch (...) {
    g_log.information("Cannot load the instrument definition.");
  }
}

void LoadILLSANS::moveDetectors(const DetectorPosition &detPos) {

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
 */
void LoadILLSANS::moveDetectorDistance(double distance,
                                       const std::string &componentName) {

  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
  try {
    mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    mover->setProperty("ComponentName", componentName);
    mover->setProperty("X", pos.X());
    mover->setProperty("Y", pos.Y());
    mover->setProperty("Z", distance);
    mover->setProperty("RelativePosition", false);
    mover->executeAsChildAlg();
    g_log.debug() << "Moving component '" << componentName
                  << "' to Z = " << distance << '\n';
  } catch (std::exception &e) {
    g_log.error() << "Cannot move the component '" << componentName
                  << "' to Z = " << distance << '\n';
    g_log.error() << e.what() << '\n';
  }
}

/**
 * Move detectors in X
 */
void LoadILLSANS::moveDetectorHorizontal(double shift,
                                         const std::string &componentName) {

  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
  try {
    mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    mover->setProperty("ComponentName", componentName);
    mover->setProperty("X", shift);
    mover->setProperty("Y", pos.Y());
    mover->setProperty("Z", pos.Z());
    mover->setProperty("RelativePosition", false);
    mover->executeAsChildAlg();
    g_log.debug() << "Moving component '" << componentName
                  << "' to X = " << shift << '\n';
  } catch (std::exception &e) {
    g_log.error() << "Cannot move the component '" << componentName
                  << "' to X = " << shift << '\n';
    g_log.error() << e.what() << '\n';
  }
}

void LoadILLSANS::moveDetectorVertical(double shift,
                                       const std::string &componentName) {

  API::IAlgorithm_sptr mover = createChildAlgorithm("MoveInstrumentComponent");
  V3D pos = getComponentPosition(componentName);
  try {
    mover->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    mover->setProperty("ComponentName", componentName);
    mover->setProperty("X", pos.X());
    mover->setProperty("Y", shift);
    mover->setProperty("Z", pos.Z());
    mover->setProperty("RelativePosition", false);
    mover->executeAsChildAlg();
    g_log.debug() << "Moving component '" << componentName
                  << "' to Y = " << shift << '\n';
  } catch (std::exception &e) {
    g_log.error() << "Cannot move the component '" << componentName
                  << "' to Y = " << shift << '\n';
    g_log.error() << e.what() << '\n';
  }
}

/**
 * Get position in space of a componentName
 */
V3D LoadILLSANS::getComponentPosition(const std::string &componentName) {
  Geometry::Instrument_const_sptr instrument =
      m_localWorkspace->getInstrument();
  Geometry::IComponent_const_sptr component =
      instrument->getComponentByName(componentName);
  return component->getPos();
}

/*
 * Loads metadata present in the nexus file
 */
void LoadILLSANS::loadMetaData(const NeXus::NXEntry &entry,
                               const std::string &instrumentNamePath) {

  g_log.debug("Loading metadata...");

  API::Run &runDetails = m_localWorkspace->mutableRun();

  int runNum = entry.getInt("run_number");
  std::string run_num = std::to_string(runNum);
  runDetails.addProperty("run_number", run_num);

  if (entry.getFloat("mode") == 0.0) { // Not TOF
    runDetails.addProperty<std::string>("tof_mode", "Non TOF");
  } else {
    runDetails.addProperty<std::string>("tof_mode", "TOF");
  }

  std::string desc =
      m_loader.getStringFromNexusPath(entry, "sample_description");
  runDetails.addProperty("sample_description", desc);

  std::string start_time = entry.getString("start_time");
  start_time = m_loader.dateTimeInIsoFormat(start_time);
  runDetails.addProperty("run_start", start_time);

  std::string end_time = entry.getString("end_time");
  end_time = m_loader.dateTimeInIsoFormat(end_time);
  runDetails.addProperty("run_end", end_time);

  double duration = entry.getFloat("duration");
  runDetails.addProperty("timer", duration);

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
}

/**
 * @param lambda : wavelength in Amstrongs
 * @param twoTheta : twoTheta in degreess
 */
double LoadILLSANS::calculateQ(const double lambda,
                               const double twoTheta) const {
  return (4 * M_PI * std::sin(twoTheta * (M_PI / 180) / 2)) / (lambda);
}

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
    } else
      g_log.debug() << "Detector " << i
                    << " is a Monitor : " << spectrumInfo.detector(i).getID()
                    << '\n';
  }

  g_log.debug() << "Calculating Qmin Qmax. Done : [" << min << "," << max
                << "]\n";

  return std::pair<double, double>(min, max);
}

void LoadILLSANS::setFinalProperties() {
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty("is_frame_skipping", 0);

  std::pair<double, double> minmax = LoadILLSANS::calculateQMaxQMin();
  runDetails.addProperty("qmin", minmax.first);
  runDetails.addProperty("qmax", minmax.second);
}

} // namespace DataHandling
} // namespace Mantid
