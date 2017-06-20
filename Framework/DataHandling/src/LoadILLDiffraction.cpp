#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/H5Util.h"
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
    : IFileLoader<NexusDescriptor>(), m_instNames({"D20"}) {}

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

  loadScanVars();
  progress.report("Loaded the scanned variables");

  loadDataScan();
  progress.report("Loaded the detector scan data");

  loadMetaData();
  progress.report("Loaded the metadata");

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
  m_numberDetectorsRead = static_cast<size_t>(data.dim1());
  m_numberScanPoints = static_cast<size_t>(data.dim0());

  // set which scanned variables are scanned, which should be the axis
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    m_scanVar[i].setAxis(axis[static_cast<int>(i)]);
    m_scanVar[i].setScanned(scanned[static_cast<int>(i)]);
  }

  resolveScanType();

  resolveInstrument();

  initWorkspace();

  fillDataScanMetaData(scan);

  if (m_scanType == DetectorScan) {
    fillMovingInstrumentScan(data, scan);
  } else {
    fillStaticInstrumentScan(data, scan, twoTheta0);
  }

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
 * Fills the loaded data to the workspace when the detector
 * is not moving during the run, but can be moved before
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
  for (size_t i = 1; i <= m_numberDetectorsActual; ++i) {
    auto &spectrum = m_outWorkspace->mutableY(i);
    auto &errors = m_outWorkspace->mutableE(i);
    for (size_t j = 0; j < m_numberScanPoints; ++j) {
      unsigned int y =
          data(static_cast<int>(j), static_cast<int>(i - 1 + deadOffset));
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
 * Returns the monitor spectrum
 * @param scan : scan data
 * @return monitor spectrum
 */
std::vector<double> LoadILLDiffraction::getMonitor(const NXDouble &scan) const {

  std::vector<double> monitor = {0.};
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if (m_scanVar[i].name == "Monitor1") {
      monitor.assign(scan() + m_numberScanPoints * i,
                     scan() + m_numberScanPoints * (i + 1));
      break;
    }
  }
  return monitor;
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
      switch (m_numberDetectorsRead) {
      // Here we have to hardcode the numbers of pixels.
      // The only way is to read the size of the detectors read from the files
      // and based on it decide which of the 3 alternative IDFs to load.
      // Some amount of pixels are dead on each end, these has to be subtracted
      // correspondingly dependent on the resolution mode
      case D20_NUMBER_PIXELS: {
        // low resolution mode
        m_instName += "_lr";
        m_numberDetectorsActual =
            D20_NUMBER_PIXELS - 2 * D20_NUMBER_DEAD_PIXELS;
        break;
      }
      case 2 * D20_NUMBER_PIXELS: {
        // nominal resolution
        m_numberDetectorsActual =
            2 * (D20_NUMBER_PIXELS - 2 * D20_NUMBER_DEAD_PIXELS);
        break;
      }
      case 3 * D20_NUMBER_PIXELS: {
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
  }
}

/**
 * Initializes the output workspace based on the resolved instrument, scan
 * points, and scan type
 */
void LoadILLDiffraction::initWorkspace() {

  size_t nSpectra = m_numberDetectorsActual + 1, nBins = 1;

  if (m_scanType == DetectorScan) {
    nSpectra *= m_numberScanPoints;
  } else if (m_scanType == OtherScan) {
    nBins = m_numberScanPoints;
  }

  m_outWorkspace = WorkspaceFactory::Instance().create("Workspace2D", nSpectra,
                                                       nBins, nBins);
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
* Rotates the detector to the 2theta0 read from the file
*/
void LoadILLDiffraction::moveTwoThetaZero(double twoTheta0) {

  Instrument_const_sptr instrument = m_outWorkspace->getInstrument();
  IComponent_const_sptr component = instrument->getComponentByName("detector");

  Quat rotation(twoTheta0, V3D(0, 1, 0));

  g_log.debug() << "Setting 2theta0 to " << twoTheta0;

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
