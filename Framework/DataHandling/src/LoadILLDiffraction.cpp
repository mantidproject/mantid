#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataObjects/ScanningWorkspaceBuilder.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/make_unique.h"

#include <boost/algorithm/string/predicate.hpp>
#include <numeric>

#include <H5Cpp.h>
#include <nexus/napi.h>
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Geometry;
using namespace H5;
using namespace Kernel;
using namespace NeXus;

namespace {
// This defines the number of physical pixels in D20 (low resolution mode)
// Then each pixel can be split into 2 (nominal) or 3 (high resolution) by DAQ
constexpr size_t D20_NUMBER_PIXELS = 1600;
// This defines the number of dead pixels on each side in low resolution mode
constexpr size_t D20_NUMBER_DEAD_PIXELS = 32;
// This defines the number of monitors in the instrument. If there are cases
// where this is no longer one this decleration should be moved.
constexpr size_t NUMBER_MONITORS = 1;
// This is the angular size of a pixel in degrees (in low resolution mode)
constexpr double D20_PIXEL_SIZE = 0.1;
constexpr double rad2deg = 180. / M_PI;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLDiffraction)

/// Returns confidence. @see IFileLoader::confidence
int LoadILLDiffraction::confidence(NexusDescriptor &descriptor) const {

  // fields existent only at the ILL Diffraction
  if (descriptor.pathExists("/entry0/data_scan")) {
    return 80;
  } else {
    return 0;
  }
}

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLDiffraction::name() const {
  return "LoadILLDiffraction";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLDiffraction::category() const {
  return "DataHandling\\Nexus;ILL\\Diffraction";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLDiffraction::summary() const {
  return "Loads ILL diffraction nexus files.";
}

/**
 * Constructor
 */
LoadILLDiffraction::LoadILLDiffraction()
    : IFileLoader<NexusDescriptor>(), m_instNames({"D20", "D2B"}) {}

/**
 * Initialize the algorithm's properties.
 */
void LoadILLDiffraction::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "File path of the data file to load");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
}

/**
 * Executes the algorithm.
 */
void LoadILLDiffraction::exec() {

  Progress progress(this, 0, 1, 3);

  m_fileName = getPropertyValue("Filename");

  progress.report("Loading the scanned variables");
  loadScanVars();

  progress.report("Loading the detector scan data");
  loadDataScan();

  progress.report("Loading the metadata");
  loadMetaData();

  setProperty("OutputWorkspace", m_outWorkspace);
}

/**
* Loads the scanned detector data
*/
void LoadILLDiffraction::loadDataScan() {

  // open the root entry
  NXRoot dataRoot(m_fileName);
  NXEntry firstEntry = dataRoot.openFirstEntry();

  m_instName = firstEntry.getString("instrument/name");

  m_startTime = DateAndTime(
      m_loadHelper.dateTimeInIsoFormat(firstEntry.getString("start_time")));

  // read the detector data
  NXData dataGroup = firstEntry.openNXData("data_scan/detector_data");
  NXUInt data = dataGroup.openUIntData();
  data.load();

  // read the scan data
  NXData scanGroup = firstEntry.openNXData("data_scan/scanned_variables");
  NXDouble scan = scanGroup.openDoubleData();
  scan.load();

  // read which variables are scanned
  NXInt scanned = firstEntry.openNXInt(
      "data_scan/scanned_variables/variables_names/scanned");
  scanned.load();

  // read what is going to be the axis
  NXInt axis =
      firstEntry.openNXInt("data_scan/scanned_variables/variables_names/axis");
  axis.load();

  // read the starting two theta
  NXFloat twoTheta0 = firstEntry.openNXFloat("instrument/2theta/value");
  twoTheta0.load();

  // figure out the dimensions
  m_sizeDim1 = static_cast<size_t>(data.dim1());
  m_sizeDim2 = static_cast<size_t>(data.dim2());
  m_numberDetectorsRead = m_sizeDim1 * m_sizeDim2;
  m_numberScanPoints = static_cast<size_t>(data.dim0());
  g_log.debug() << "Read " << m_numberDetectorsRead << " detectors and "
                << m_numberScanPoints << "\n";

  // set which scanned variables are scanned, which should be the axis
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    m_scanVar[i].setAxis(axis[static_cast<int>(i)]);
    m_scanVar[i].setScanned(scanned[static_cast<int>(i)]);
  }

  resolveScanType();

  resolveInstrument();

  if (m_scanType == DetectorScan) {
    initMovingWorkspace(scan);
    fillMovingInstrumentScan(data, scan);
  } else {
    initStaticWorkspace();
    fillStaticInstrumentScan(data, scan, twoTheta0);
  }

  fillDataScanMetaData(scan);

  scanGroup.close();
  dataGroup.close();
  firstEntry.close();
  dataRoot.close();
}

/**
* Dumps the metadata from the whole file to SampleLogs
*/
void LoadILLDiffraction::loadMetaData() {

  m_outWorkspace->mutableRun().addProperty("Facility", std::string("ILL"));

  // Open NeXus file
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(m_fileName.c_str(), NXACC_READ, &nxHandle);

  if (nxStat != NX_ERROR) {
    m_loadHelper.addNexusFieldsToWsRun(nxHandle, m_outWorkspace->mutableRun());
    nxStat = NXclose(&nxHandle);
  }
}

/**
 * Initializes the output workspace based on the resolved instrument, scan
 * points, and scan type
 */
void LoadILLDiffraction::initStaticWorkspace() {
  size_t nSpectra = m_numberDetectorsActual + NUMBER_MONITORS;
  size_t nBins = 1;

  if (m_scanType == DetectorScan) {
    nSpectra *= m_numberScanPoints;
  } else if (m_scanType == OtherScan) {
    nBins = m_numberScanPoints;
  }

  m_outWorkspace = WorkspaceFactory::Instance().create("Workspace2D", nSpectra,
                                                       nBins, nBins);
}

/**
 * Use the ScanningWorkspaceBuilder to create a time indexed workspace.
 *
 * @param scan : scan data
 */
void LoadILLDiffraction::initMovingWorkspace(const NXDouble &scan) {
  const size_t nTimeIndexes = m_numberScanPoints;
  const size_t nBins = 1;
  const bool isPointData = true;

  const auto instrumentWorkspace = loadEmptyInstrument();
  const auto &instrument = instrumentWorkspace->getInstrument();

  auto scanningWorkspaceBuilder = DataObjects::ScanningWorkspaceBuilder(
      instrument, nTimeIndexes, nBins, isPointData);

  std::vector<double> timeDurations =
      getScannedVaribleByPropertyName(scan, "Time");
  scanningWorkspaceBuilder.setTimeRanges(m_startTime, timeDurations);

  g_log.debug() << "First time index starts at:"
                << m_startTime.toISO8601String() << "\n";

  g_log.debug() << "Last time index ends at:"
                << (m_startTime + std::accumulate(timeDurations.begin(),
                                                  timeDurations.end(), 0.0))
                       .toISO8601String() << "\n";

  // Angles in the NeXus files are the absolute position for tube 1
  std::vector<double> tubeAngles =
      getScannedVaribleByPropertyName(scan, "Position");

  const auto &referenceComponentPosition =
      getReferenceComponentPosition(instrumentWorkspace);

  // Convert the tube positions to relative rotations for all detectors
  calculateRelativeRotations(tubeAngles, referenceComponentPosition);

  auto rotationCentre = V3D(0, 0, 0);
  auto rotationAxis = V3D(0, 1, 0);
  scanningWorkspaceBuilder.setRelativeRotationsForScans(
      std::move(tubeAngles), rotationCentre, rotationAxis);

  m_outWorkspace = scanningWorkspaceBuilder.buildWorkspace();
}

/**
 * Get the position of the component in the workspace which corresponds to the
 *angle stored in the scanned variables of the NeXus files. For 1D detectors
 *this should be the first detector (ID 1), while for 2D detectors (D2B only) it
 *should be the position of the first tube.
 *
 * @param instrumentWorkspace The empty workspace containing the instrument
 * @return A V3D object containing the position of the relevant component
 */
V3D LoadILLDiffraction::getReferenceComponentPosition(
    const MatrixWorkspace_sptr &instrumentWorkspace) {
  if (m_instName == "D2B") {
    return instrumentWorkspace->getInstrument()
        ->getComponentByName("tube_1")
        ->getPos();
  }

  const auto &detInfo = instrumentWorkspace->detectorInfo();
  const auto &indexOfFirstDet = detInfo.indexOf(1);
  return detInfo.position(indexOfFirstDet);
}

/**
 * Convert from absolute rotation angle, around the sample, of tube 1, to a
 *relative rotation angle around the sample.
 *
 * @param tubeRotations Input is the absolute rotations around the sample of
 *tube 1, output is the relative rotations required from the IDF for all
 *detectors
 * @param firstTubePosition A V3D object containing the position of the first
 *tube
 */
void LoadILLDiffraction::calculateRelativeRotations(
    std::vector<double> &tubeRotations, const V3D &firstTubePosition) {
  // The rotations in the NeXus file are the absolute rotation of the first
  // tube. Here we get the angle of that tube as defined in the IDF.

  double firstTubeRotationAngle =
      firstTubePosition.angle(V3D(0, 0, 1)) * rad2deg;

  if (m_instName == "D20")
    firstTubeRotationAngle += D20_NUMBER_DEAD_PIXELS * D20_PIXEL_SIZE;

  g_log.debug() << "First tube rotation:" << firstTubeRotationAngle << "\n";

  // Now pass calculate the rotations to apply for each time index.
  std::transform(
      tubeRotations.begin(), tubeRotations.end(), tubeRotations.begin(),
      [&](double angle) { return (angle - firstTubeRotationAngle); });

  g_log.debug() << "Instrument rotations to be applied : "
                << tubeRotations.front() << " to " << tubeRotations.back()
                << "\n";
}

/**
 * Fills the counts for the instrument with moving detectors.
 *
 * @param data : detector data
 * @param scan : scan data
 */
void LoadILLDiffraction::fillMovingInstrumentScan(const NXUInt &data,
                                                  const NXDouble &scan) {

  std::vector<double> axis = {0.};
  std::vector<double> monitor = getMonitor(scan);

  // First load the monitors
  for (size_t i = 0; i < NUMBER_MONITORS; ++i) {
    for (size_t j = 0; j < m_numberScanPoints; ++j) {
      const auto wsIndex = j + i * m_numberScanPoints;
      m_outWorkspace->mutableY(wsIndex) = monitor[j];
      m_outWorkspace->mutableE(wsIndex) = sqrt(monitor[j]);
      m_outWorkspace->mutableX(wsIndex) = axis;
    }
  }

  // Dead pixel offset, should be zero except for D20
  size_t deadOffset = (m_numberDetectorsRead - m_numberDetectorsActual) / 2;

  // Then load the detector spectra
  for (size_t i = NUMBER_MONITORS;
       i < m_numberDetectorsActual + NUMBER_MONITORS; ++i) {
    for (size_t j = 0; j < m_numberScanPoints; ++j) {
      const auto tubeNumber = (i - NUMBER_MONITORS + deadOffset) / m_sizeDim1;
      const auto pixelInTubeNumber = (i - NUMBER_MONITORS) % m_sizeDim2;
      unsigned int y = data(static_cast<int>(j), static_cast<int>(tubeNumber),
                            static_cast<int>(pixelInTubeNumber));
      const auto wsIndex = j + i * m_numberScanPoints;
      m_outWorkspace->mutableY(wsIndex) = y;
      m_outWorkspace->mutableE(wsIndex) = sqrt(y);
      m_outWorkspace->mutableX(wsIndex) = axis;
    }
  }
}

/**
 * Fills the loaded data to the workspace when the detector
 * is not moving during the run, but can be moved before
 *
 * @param data : detector data
 * @param scan : scan data
 * @param twoTheta0 : starting two theta
 */
void LoadILLDiffraction::fillStaticInstrumentScan(const NXUInt &data,
                                                  const NXDouble &scan,
                                                  const NXFloat &twoTheta0) {

  std::vector<double> axis = getAxis(scan);
  std::vector<double> monitor = getMonitor(scan);

  // Assign monitor counts
  m_outWorkspace->mutableX(0) = axis;
  m_outWorkspace->mutableY(0) = monitor;
  std::transform(monitor.begin(), monitor.end(),
                 m_outWorkspace->mutableE(0).begin(),
                 [](double e) { return sqrt(e); });

  // Assign detector counts
  size_t deadOffset = (m_numberDetectorsRead - m_numberDetectorsActual) / 2;
  for (size_t i = NUMBER_MONITORS;
       i < m_numberDetectorsActual + NUMBER_MONITORS; ++i) {
    auto &spectrum = m_outWorkspace->mutableY(i);
    auto &errors = m_outWorkspace->mutableE(i);
    const auto tubeNumber = (i - NUMBER_MONITORS + deadOffset) / m_sizeDim1;
    const auto pixelInTubeNumber = (i - NUMBER_MONITORS) % m_sizeDim2;
    for (size_t j = 0; j < m_numberScanPoints; ++j) {
      unsigned int y = data(static_cast<int>(j), static_cast<int>(tubeNumber),
                            static_cast<int>(pixelInTubeNumber));
      spectrum[j] = y;
      errors[j] = sqrt(y);
    }
    m_outWorkspace->mutableX(i) = axis;
  }

  // Link the instrument
  loadStaticInstrument();

  // Move to the starting 2theta
  moveTwoThetaZero(double(twoTheta0[0]));
}

/**
 * Loads the scanned_variables/variables_names block
 */
void LoadILLDiffraction::loadScanVars() {

  H5File h5file(m_fileName, H5F_ACC_RDONLY);

  Group entry0 = h5file.openGroup("entry0");
  Group dataScan = entry0.openGroup("data_scan");
  Group scanVar = dataScan.openGroup("scanned_variables");
  Group varNames = scanVar.openGroup("variables_names");

  const auto names = H5Util::readStringVector(varNames, "name");
  const auto properties = H5Util::readStringVector(varNames, "property");
  const auto units = H5Util::readStringVector(varNames, "unit");

  for (size_t i = 0; i < names.size(); ++i) {
    m_scanVar.emplace_back(ScannedVariables(names[i], properties[i], units[i]));
  }

  varNames.close();
  scanVar.close();
  dataScan.close();
  entry0.close();
  h5file.close();
}

/**
 * Creates time series sample logs for the scanned variables
 * @param scan : scan data
 */
void LoadILLDiffraction::fillDataScanMetaData(const NXDouble &scan) {
  auto absoluteTimes = getAbsoluteTimes(scan);
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if (m_scanVar[i].axis != 1 &&
        !boost::starts_with(m_scanVar[i].property, "Monitor")) {
      auto property =
          Kernel::make_unique<TimeSeriesProperty<double>>(m_scanVar[i].name);
      for (size_t j = 0; j < m_numberScanPoints; ++j) {
        property->addValue(absoluteTimes[j],
                           scan(static_cast<int>(i), static_cast<int>(j)));
      }
      m_outWorkspace->mutableRun().addLogData(std::move(property));
    }
  }
}

/**
 * Gets a scanned variable based on its property type in the scanned_variables
 *block.
 *
 * @param scan : scan data
 * @param propertyName The name of the property
 * @return A vector of doubles containing the scanned variable
 * @throw runtime_error If a scanned variable property name is missing from the
 *NeXus file
 */
std::vector<double> LoadILLDiffraction::getScannedVaribleByPropertyName(
    const NXDouble &scan, const std::string &propertyName) const {
  std::vector<double> scannedVariable;

  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if (m_scanVar[i].property.compare(propertyName) == 0) {
      for (size_t j = 0; j < m_numberScanPoints; ++j) {
        scannedVariable.push_back(
            scan(static_cast<int>(i), static_cast<int>(j)));
      }
      break;
    }
  }

  if (scannedVariable.size() == 0)
    throw std::runtime_error(
        "Can not load file because scanned variable with property name " +
        propertyName + " was not found");

  return scannedVariable;
}

/**
 * Returns the monitor spectrum
 * @param scan : scan data
 * @return monitor spectrum
 * @throw std::runtime_error If there are no entries named Monitor1 or Monitor_1
 * in the NeXus file
 */
std::vector<double> LoadILLDiffraction::getMonitor(const NXDouble &scan) const {

  std::vector<double> monitor = {0.};
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if ((m_scanVar[i].name == "Monitor1") ||
        (m_scanVar[i].name == "Monitor_1")) {
      monitor.assign(scan() + m_numberScanPoints * i,
                     scan() + m_numberScanPoints * (i + 1));
      return monitor;
    }
  }
  throw std::runtime_error("Monitors not found in scanned variables");
}

/**
 * Returns the x-axis
 * @param scan : scan data
 * @return the x-axis
 */
std::vector<double> LoadILLDiffraction::getAxis(const NXDouble &scan) const {

  std::vector<double> axis = {0.};
  if (m_scanType == OtherScan) {
    for (size_t i = 0; i < m_scanVar.size(); ++i) {
      if (m_scanVar[i].axis == 1) {
        axis.assign(scan() + m_numberScanPoints * i,
                    scan() + m_numberScanPoints * (i + 1));
        break;
      }
    }
  }
  return axis;
}

/**
 * Returns the durations for each scan point
 * @param scan : scan data
 * @return vector of durations
 */
std::vector<double>
LoadILLDiffraction::getDurations(const NXDouble &scan) const {
  std::vector<double> timeDurations;
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if (boost::starts_with(m_scanVar[i].property, "Time")) {
      timeDurations.assign(scan() + m_numberScanPoints * i,
                           scan() + m_numberScanPoints * (i + 1));
      break;
    }
  }
  return timeDurations;
}

/**
 * Returns the vector of absolute times for each scan point
 * @param scan : scan data
 * @return vector of absolute times
 */
std::vector<DateAndTime>
LoadILLDiffraction::getAbsoluteTimes(const NXDouble &scan) const {
  std::vector<DateAndTime> times; // in units of ns
  std::vector<double> durations = getDurations(scan);
  DateAndTime time = m_startTime;
  times.emplace_back(time);
  size_t timeIndex = 1;
  while (timeIndex < m_numberScanPoints) {
    time += durations[timeIndex - 1] * 1E9; // from sec to ns
    times.push_back(time);
    ++timeIndex;
  }
  return times;
}

/**
 * Resolves the scan type
 */
void LoadILLDiffraction::resolveScanType() {
  ScanType result = NoScan;
  for (const auto &scanVar : m_scanVar) {
    if (scanVar.scanned == 1) {
      result = OtherScan;
      if (scanVar.name == "2theta") {
        result = DetectorScan;
        break;
      }
    }
  }
  m_scanType = result;
}

/**
 * Resolves the instrument based on instrument name and resolution mode
 * @throws runtime_error, if the instrument is not supported
 */
void LoadILLDiffraction::resolveInstrument() {
  if (m_instNames.find(m_instName) == m_instNames.end()) {
    throw std::runtime_error("Instrument " + m_instName + " not supported.");
  } else {
    m_numberDetectorsActual = m_numberDetectorsRead;
    if (m_instName == "D20") {
      // Here we have to hardcode the numbers of pixels.
      // The only way is to read the size of the detectors read from the files
      // and based on it decide which of the 3 alternative IDFs to load.
      // Some amount of pixels are dead on each end, these have to be
      // subtracted
      // correspondingly dependent on the resolution mode
      m_resolutionMode = m_numberDetectorsRead / D20_NUMBER_PIXELS;
      size_t activePixels = D20_NUMBER_PIXELS - 2 * D20_NUMBER_DEAD_PIXELS;
      m_numberDetectorsActual = m_resolutionMode * activePixels;
      // 1: low resolution, 2: nominal, 3: high resolution
      switch (m_resolutionMode) {
      case 1: {
        // low resolution mode
        m_instName += "_lr";
        m_numberDetectorsActual =
            D20_NUMBER_PIXELS - 2 * D20_NUMBER_DEAD_PIXELS;
        break;
      }
      case 2: {
        // nominal resolution
        m_numberDetectorsActual =
            2 * (D20_NUMBER_PIXELS - 2 * D20_NUMBER_DEAD_PIXELS);
        break;
      }
      case 3: {
        // high resolution mode
        m_instName += "_hr";
        m_numberDetectorsActual =
            3 * (D20_NUMBER_PIXELS - 2 * D20_NUMBER_DEAD_PIXELS);
        break;
      }
      default:
        throw std::runtime_error("Unknown resolution mode for instrument " +
                                 m_instName);
      }
    }
    g_log.debug() << "Instrument name is " << m_instName << " and has "
                  << m_numberDetectorsActual << " actual detectors.\n";
  }
}

/**
* Runs LoadInstrument as child to link the non-moving instrument to workspace
*/
void LoadILLDiffraction::loadStaticInstrument() {
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("Filename", getInstrumentFilePath(m_instName));
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_outWorkspace);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->execute();
}

/**
 * Runs LoadEmptyInstrument and returns a workspace with the instrument, to be
 *used in the ScanningWorkspaceBuilder.
 *
 * @return A MatrixWorkspace containing the correct instrument
 */
MatrixWorkspace_sptr LoadILLDiffraction::loadEmptyInstrument() {
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadEmptyInstrument");
  loadInst->setPropertyValue("InstrumentName", m_instName);
  loadInst->execute();

  return loadInst->getProperty("OutputWorkspace");
}

/**
* Rotates the detector to the 2theta0 read from the file
* @param twoTheta0Read : 2theta0 read from the file
*/
void LoadILLDiffraction::moveTwoThetaZero(double twoTheta0Read) {
  Instrument_const_sptr instrument = m_outWorkspace->getInstrument();
  IComponent_const_sptr component = instrument->getComponentByName("detector");
  double twoTheta0Actual = twoTheta0Read;
  if (m_instName == "D20") {
    twoTheta0Actual += D20_NUMBER_DEAD_PIXELS * D20_PIXEL_SIZE;
  }
  Quat rotation(twoTheta0Actual, V3D(0, 1, 0));
  g_log.debug() << "Setting 2theta0 to " << twoTheta0Actual;
  m_outWorkspace->mutableDetectorInfo().setRotation(*component, rotation);
}

/**
* Makes up the full path of the relevant IDF dependent on resolution mode
* @param instName : the name of the instrument (including the resolution mode
* suffix)
* @return : the full path to the corresponding IDF
*/
std::string
LoadILLDiffraction::getInstrumentFilePath(const std::string &instName) const {

  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(instName + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

} // namespace DataHandling
} // namespace Mantid
