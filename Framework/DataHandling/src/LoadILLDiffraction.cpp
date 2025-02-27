// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLDiffraction.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataObjects/ScanningWorkspaceBuilder.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile.hpp"

#include <H5Cpp.h>
#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/round.hpp>
#include <numeric>

namespace Mantid::DataHandling {

using namespace API;
using namespace Geometry;
using namespace H5;
using namespace Kernel;
using namespace NeXus;
using Types::Core::DateAndTime;

namespace {
// This defines the number of physical pixels in D20 (low resolution mode)
// Then each pixel can be split into 2 (nominal) or 3 (high resolution) by DAQ
constexpr size_t D20_NUMBER_PIXELS = 1600;
// This defines the number of dead pixels on each side in low resolution mode
constexpr size_t D20_NUMBER_DEAD_PIXELS = 32;
// This defines the number of monitors in the instrument. If there are cases
// where this is no longer one this decleration should be moved.
constexpr int NUMBER_MONITORS = 1;
// This is the angular size of a pixel in degrees (in low resolution mode)
constexpr double D20_PIXEL_SIZE = 0.1;
// The conversion factor from radian to degree
constexpr double RAD_TO_DEG = 180. / M_PI;
// A factor to compute E from lambda: E (mev) = waveToE/lambda(A)
constexpr double WAVE_TO_E = 81.8;
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLDiffraction)

/// Returns confidence. @see IFileLoader::confidence
int LoadILLDiffraction::confidence(NexusDescriptor &descriptor) const {

  // fields existent only at the ILL Diffraction
  // the second one is to recognize D1B, Tx field eliminates SALSA
  // the third one is to recognize IN5/PANTHER/SHARP scan mode
  if ((descriptor.isEntry("/entry0/instrument/2theta") && !descriptor.isEntry("/entry0/instrument/Tx")) ||
      descriptor.isEntry("/entry0/instrument/Canne")) {
    return 80;
  } else {
    return 0;
  }
}

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLDiffraction::name() const { return "LoadILLDiffraction"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLDiffraction::category() const { return "DataHandling\\Nexus;ILL\\Diffraction"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLDiffraction::summary() const { return "Loads ILL diffraction nexus files."; }

/**
 * Constructor
 */
LoadILLDiffraction::LoadILLDiffraction()
    : IFileLoader<NexusDescriptor>(), m_instNames({"D20", "D2B", "D1B", "D4C", "IN5", "PANTHER", "SHARP"}) {}
/**
 * Initialize the algorithm's properties.
 */
void LoadILLDiffraction::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "File path of the data file to load");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  declareProperty("TwoThetaOffset", 0.0, "2 theta offset for D1B data, in degrees.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("AlignTubes", false, Direction::Input),
                  "Apply vertical and horizontal alignment of tubes as defined in IPF");
  declareProperty("ConvertAxisAndTranspose", false,
                  "Whether to convert the spectrum axis to 2theta and "
                  "transpose (for 1D detector and no-scan configuration)");
}

std::map<std::string, std::string> LoadILLDiffraction::validateInputs() {
  std::map<std::string, std::string> issues;
  return issues;
}

/**
 * Executes the algorithm.
 */
void LoadILLDiffraction::exec() {

  Progress progress(this, 0, 1, 4);

  m_filename = getPropertyValue("Filename");

  m_scanVar.clear();
  progress.report("Loading the scanned variables");
  loadScanVars();

  progress.report("Loading the detector scan data");
  loadDataScan();

  progress.report("Loading the metadata");
  loadMetaData();

  progress.report("Setting additional sample logs");
  setSampleLogs();

  if (m_instName != "D2B" && m_scanType != DetectorScan && getProperty("ConvertAxisAndTranspose"))
    convertAxisAndTranspose();

  setProperty("OutputWorkspace", m_outWorkspace);
}

/**
 * Loads the scanned detector data
 */
void LoadILLDiffraction::loadDataScan() {

  // open the root entry
  NXRoot dataRoot(m_filename);
  NXEntry firstEntry = dataRoot.openFirstEntry();
  m_instName = firstEntry.getString("instrument/name");
  m_startTime = DateAndTime(LoadHelper::dateTimeInIsoFormat(firstEntry.getString("start_time")));
  // Load the data
  std::string dataName = "data_scan/detector_data/data";
  g_log.notice() << "Loading data from " + dataName;
  auto data = firstEntry.openNXDataSet<int>(dataName);
  data.load();

  // read the scan data
  auto scan = LoadHelper::getDoubleDataset(firstEntry, "data_scan/scanned_variables");
  scan.load();

  // read which variables are scanned
  NXInt scanned = firstEntry.openNXInt("data_scan/scanned_variables/variables_names/scanned");
  scanned.load();

  // read what is going to be the axis
  NXInt axis = firstEntry.openNXInt("data_scan/scanned_variables/variables_names/axis");
  axis.load();

  // read the starting two theta
  double twoThetaValue = 0;
  if (m_instName == "D1B") {
    if (getPointerToProperty("TwoThetaOffset")->isDefault()) {
      g_log.notice("A 2theta offset angle is necessary for D1B data.");
      twoThetaValue = 0;
    } else {
      twoThetaValue = getProperty("TwoThetaOffset");
    }
  } else {
    std::string twoThetaPath = "instrument/2theta/value";
    NXFloat twoTheta0 = firstEntry.openNXFloat(twoThetaPath);
    twoTheta0.load();
    twoThetaValue = double(twoTheta0[0]);
  }

  // figure out the dimensions
  m_sizeDim1 = static_cast<size_t>(data.dim1());
  m_sizeDim2 = static_cast<size_t>(data.dim2());
  m_numberDetectorsRead = m_sizeDim1 * m_sizeDim2;
  m_numberScanPoints = static_cast<size_t>(data.dim0());
  g_log.debug() << "Read " << m_numberDetectorsRead << " detectors and " << m_numberScanPoints << "\n";

  // set which scanned variables are scanned, which should be the axis
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    m_scanVar[i].setAxis(axis[static_cast<int>(i)]);
    m_scanVar[i].setScanned(scanned[static_cast<int>(i)]);
  }

  resolveInstrument();
  resolveScanType();
  computeThetaOffset();

  if (m_scanType == DetectorScan) {
    initMovingWorkspace(scan);
    fillMovingInstrumentScan(data, scan);
  } else {
    initStaticWorkspace();
    fillStaticInstrumentScan(data, scan, twoThetaValue);
  }

  fillDataScanMetaData(scan);

  firstEntry.close();
  dataRoot.close();
}

/**
 * Dumps the metadata from the whole file to SampleLogs
 */
void LoadILLDiffraction::loadMetaData() {

  auto &mutableRun = m_outWorkspace->mutableRun();
  mutableRun.addProperty("Facility", std::string("ILL"));

  // get some information from the NeXus file
  try {
    ::NeXus::File filehandle(m_filename, NXACC_READ);
    LoadHelper::addNexusFieldsToWsRun(filehandle, mutableRun);
  } catch (const ::NeXus::Exception &e) {
    g_log.debug() << "Failed to open nexus file \"" << m_filename << "\" in read mode: " << e.what() << "\n";
  }

  if (mutableRun.hasProperty("run_number"))
    mutableRun.addProperty("run_list", mutableRun.getPropertyValueAsType<int>("run_number"));
  else
    throw std::runtime_error("Failed to find run_number in Run object");

  if (!mutableRun.hasProperty("Detector.calibration_file"))
    mutableRun.addProperty("Detector.calibration_file", std::string("none"));
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

  m_outWorkspace = WorkspaceFactory::Instance().create("Workspace2D", nSpectra, nBins, nBins);

  // the start time is needed in the workspace when loading the parameter file
  m_outWorkspace->mutableRun().addProperty("start_time", m_startTime.toISO8601String());
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

  const auto instrumentWorkspace = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  auto &run = instrumentWorkspace->mutableRun();
  // the start time is needed in the workspace when loading the parameter file
  run.addProperty("start_time", m_startTime.toISO8601String());

  getInstrumentFilePath(m_instName);
  LoadHelper::loadEmptyInstrument(instrumentWorkspace, m_instName);
  const auto &instrument = instrumentWorkspace->getInstrument();
  auto &params = instrumentWorkspace->instrumentParameters();

  const auto &referenceComponentPosition = getReferenceComponentPosition(instrumentWorkspace);

  double refR, refTheta, refPhi;
  referenceComponentPosition.getSpherical(refR, refTheta, refPhi);

  if (m_instName == "D2B") {
    const bool doAlign = getProperty("AlignTubes");
    auto &compInfo = instrumentWorkspace->mutableComponentInfo();

    Geometry::IComponent_const_sptr detectors = instrument->getComponentByName("detectors");
    const auto detCompIndex = compInfo.indexOf(detectors->getComponentID());
    const auto tubes = compInfo.children(detCompIndex);
    const size_t nTubes = tubes.size();
    Geometry::IComponent_const_sptr tube1 = instrument->getComponentByName("tube_1");
    const auto tube1CompIndex = compInfo.indexOf(tube1->getComponentID());
    const auto pixels = compInfo.children(tube1CompIndex);
    const size_t nPixels = pixels.size();

    Geometry::IComponent_const_sptr pixel = instrument->getComponentByName("standard_pixel");
    Geometry::BoundingBox bb;
    pixel->getBoundingBox(bb);
    m_pixelHeight = bb.yMax() - bb.yMin();

    const auto tubeAnglesStr = params.getString("D2B", "tube_angles");
    if (!tubeAnglesStr.empty() && doAlign) {
      std::vector<std::string> tubeAngles;
      boost::split(tubeAngles, tubeAnglesStr[0], boost::is_any_of(","));
      const double ref = -refTheta;
      for (size_t i = 1; i <= nTubes; ++i) {
        const std::string compName = "tube_" + std::to_string(i);
        Geometry::IComponent_const_sptr component = instrument->getComponentByName(compName);
        double r, theta, phi;
        V3D oldPos = component->getPos();
        oldPos.getSpherical(r, theta, phi);
        V3D newPos;
        const double angle = std::stod(tubeAngles[i - 1]);
        const double finalAngle = fabs(ref - angle);
        g_log.debug() << "Rotating " << compName << "to " << finalAngle << "rad\n";
        newPos.spherical(r, finalAngle, phi);
        const auto componentIndex = compInfo.indexOf(component->getComponentID());
        compInfo.setPosition(componentIndex, newPos);
      }
    }

    const auto tubeCentersStr = params.getString("D2B", "tube_centers");
    if (!tubeCentersStr.empty() && doAlign) {
      std::vector<std::string> tubeCenters;
      double maxYOffset = 0.;
      boost::split(tubeCenters, tubeCentersStr[0], boost::is_any_of(","));
      for (size_t i = 1; i <= nTubes; ++i) {
        const std::string compName = "tube_" + std::to_string(i);
        Geometry::IComponent_const_sptr component = instrument->getComponentByName(compName);
        const double offset = std::stod(tubeCenters[i - 1]) - (double(nPixels) / 2 - 0.5);
        const double y = -offset * m_pixelHeight;
        V3D translation(0, y, 0);
        if (std::fabs(y) > maxYOffset) {
          maxYOffset = std::fabs(y);
        }
        g_log.debug() << "Moving " << compName << " to " << y << "\n";
        V3D pos = component->getPos() + translation;
        const auto componentIndex = compInfo.indexOf(component->getComponentID());
        compInfo.setPosition(componentIndex, pos);
      }
      m_maxHeight = double(nPixels + 1) * m_pixelHeight / 2 + maxYOffset;
    }
  }

  auto scanningWorkspaceBuilder = DataObjects::ScanningWorkspaceBuilder(instrument, nTimeIndexes, nBins, isPointData);

  std::vector<double> timeDurations = getScannedVaribleByPropertyName(scan, "Time");
  scanningWorkspaceBuilder.setTimeRanges(m_startTime, timeDurations);

  g_log.debug() << "First time index starts at:" << m_startTime.toISO8601String() << "\n";

  g_log.debug() << "Last time index ends at:"
                << (m_startTime + std::accumulate(timeDurations.begin(), timeDurations.end(), 0.0)).toISO8601String()
                << "\n";

  // Angles in the NeXus files are the absolute position for tube 1
  std::vector<double> tubeAngles = getScannedVaribleByPropertyName(scan, "Position");

  // Convert the tube positions to relative rotations for all detectors
  calculateRelativeRotations(tubeAngles, referenceComponentPosition);

  auto rotationCentre = V3D(0, 0, 0);
  auto rotationAxis = V3D(0, 1, 0);
  scanningWorkspaceBuilder.setRelativeRotationsForScans(std::move(tubeAngles), rotationCentre, rotationAxis);

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
V3D LoadILLDiffraction::getReferenceComponentPosition(const MatrixWorkspace_sptr &instrumentWorkspace) {
  if (m_instName == "D2B") {
    return instrumentWorkspace->getInstrument()->getComponentByName("tube_128")->getPos();
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
void LoadILLDiffraction::calculateRelativeRotations(std::vector<double> &tubeRotations, const V3D &firstTubePosition) {
  // The rotations in the NeXus file are the absolute rotation of the first
  // tube. Here we get the angle of that tube as defined in the IDF.

  double firstTubeRotationAngle = firstTubePosition.angle(V3D(0, 0, 1)) * RAD_TO_DEG;

  // note that for D20 we have to subtract the offset here
  // unlike in the static detector case, because in the transform
  // below, we take (angle - firstTubeRotatingAngle)
  if (m_instName == "D20") {
    firstTubeRotationAngle -= m_offsetTheta;
  } else if (m_instName == "D2B") {
    firstTubeRotationAngle = -firstTubeRotationAngle;
    std::transform(tubeRotations.begin(), tubeRotations.end(), tubeRotations.begin(),
                   [&](double angle) { return (-angle); });
  }

  g_log.debug() << "First tube rotation:" << firstTubeRotationAngle << "\n";

  // Now pass calculate the rotations to apply for each time index.
  std::transform(tubeRotations.begin(), tubeRotations.end(), tubeRotations.begin(),
                 [&](double angle) { return (angle - firstTubeRotationAngle); });

  g_log.debug() << "Instrument rotations to be applied : " << tubeRotations.front() << " to " << tubeRotations.back()
                << "\n";
}

/**
 * Fills the counts for the instrument with moving detectors.
 *
 * @param data : detector data
 * @param scan : scan data
 */
void LoadILLDiffraction::fillMovingInstrumentScan(const NXInt &data, const NXDouble &scan) {

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

  // prepare inputs, dimension orders and list of accepted IDs for exclusion of inactive detectors (D20)
  std::tuple<int, int, int> dimOrder{2, 1, 0}; // scan - tube - pixel
  std::set<int> acceptedIDs;
  if (static_cast<int>(m_numberDetectorsActual) != data.dim1() * data.dim2()) {
    for (auto index = static_cast<int>(NUMBER_MONITORS);
         index < static_cast<int>(m_numberDetectorsActual + NUMBER_MONITORS); ++index)
      acceptedIDs.insert(index);
  }
  std::vector<int> customDetectorIDs;
  // Assign detector counts
  LoadHelper::fillMovingWorkspace(m_outWorkspace, data, axis, NUMBER_MONITORS, acceptedIDs, customDetectorIDs,
                                  dimOrder);
}

/**
 * Fills the loaded data to the workspace when the detector
 * is not moving during the run, but can be moved before
 *
 * @param data : detector data
 * @param scan : scan data
 * @param twoTheta0 : starting two theta
 */
void LoadILLDiffraction::fillStaticInstrumentScan(const NXInt &data, const NXDouble &scan, const double &twoTheta0) {

  const std::vector<double> axis = getAxis(scan);
  const std::vector<double> monitor = getMonitor(scan);

  int startIndex = static_cast<int>(NUMBER_MONITORS);

  // Assign monitor counts
  m_outWorkspace->mutableX(0) = axis;
  m_outWorkspace->mutableY(0) = monitor;
  std::transform(monitor.begin(), monitor.end(), m_outWorkspace->mutableE(0).begin(), [](double e) { return sqrt(e); });

  // prepare inputs, dimension orders and list of accepted IDs for exclusion of inactive detectors (D20)
  std::tuple<int, int, int> dimOrder{2, 1, 0}; // scan - tube - pixel
  std::set<int> acceptedIDs;
  if (static_cast<int>(m_numberDetectorsActual) != data.dim1() * data.dim2()) {
    for (int i = static_cast<int>(startIndex); i < static_cast<int>(startIndex + m_numberDetectorsActual); ++i)
      acceptedIDs.insert(i);
  }
  // Assign detector counts
  LoadHelper::fillStaticWorkspace(m_outWorkspace, data, axis, startIndex, true, std::vector<int>(), acceptedIDs,
                                  dimOrder);

  // Link the instrument
  LoadHelper::loadEmptyInstrument(m_outWorkspace, m_instName);
  // Move to the starting 2theta
  moveTwoThetaZero(twoTheta0);
}

/**
 * Loads the scanned_variables/variables_names block
 */
void LoadILLDiffraction::loadScanVars() {
  H5File h5file(m_filename, H5F_ACC_RDONLY, NeXus::H5Util::defaultFileAcc());

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
  auto &mutableRun = m_outWorkspace->mutableRun();
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if (!m_scanVar[i].property.starts_with("Monitor")) {
      const std::string scanVarName = boost::algorithm::to_lower_copy(m_scanVar[i].name);
      const std::string scanVarProp = boost::algorithm::to_lower_copy(m_scanVar[i].property);
      const std::string propName = scanVarName + "." + scanVarProp;
      if (m_scanVar[i].scanned == 1) {
        mutableRun.addProperty("ScanVar", propName, true);
      }
      auto property = std::make_unique<TimeSeriesProperty<double>>(propName);
      for (size_t j = 0; j < m_numberScanPoints; ++j) {
        property->addValue(absoluteTimes[j], scan(static_cast<int>(i), static_cast<int>(j)));
      }
      mutableRun.addLogData(std::move(property), true);
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
std::vector<double> LoadILLDiffraction::getScannedVaribleByPropertyName(const NXDouble &scan,
                                                                        const std::string &propertyName) const {
  std::vector<double> scannedVariable;

  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if (m_scanVar[i].property == propertyName) {
      for (size_t j = 0; j < m_numberScanPoints; ++j) {
        scannedVariable.emplace_back(scan(static_cast<int>(i), static_cast<int>(j)));
      }
      break;
    }
  }

  if (scannedVariable.empty())
    throw std::runtime_error("Can not load file because scanned variable with property name " + propertyName +
                             " was not found");

  return scannedVariable;
}

/**
 * Returns the monitor spectrum
 * @param scan : scan data
 * @return monitor spectrum
 * @throw std::runtime_error If there are no entries named Monitor1 or
 * Monitor_1, or monitor1 in the NeXus file
 */
std::vector<double> LoadILLDiffraction::getMonitor(const NXDouble &scan) const {

  std::vector<double> monitor = {0.};
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if ((m_scanVar[i].name == "Monitor1") || (m_scanVar[i].name == "Monitor_1") || (m_scanVar[i].name == "monitor1")) {
      monitor.assign(scan() + m_numberScanPoints * i, scan() + m_numberScanPoints * (i + 1));
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
        axis.assign(scan() + m_numberScanPoints * i, scan() + m_numberScanPoints * (i + 1));
        break;
      }
    }
  }
  return axis;
}

/**
 * Returns the durations in seconds for each scan point
 * @param scan : scan data
 * @return vector of durations
 */
std::vector<double> LoadILLDiffraction::getDurations(const NXDouble &scan) const {
  std::vector<double> timeDurations;
  for (size_t i = 0; i < m_scanVar.size(); ++i) {
    if (m_scanVar[i].property.starts_with("Time")) {
      timeDurations.assign(scan() + m_numberScanPoints * i, scan() + m_numberScanPoints * (i + 1));
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
std::vector<DateAndTime> LoadILLDiffraction::getAbsoluteTimes(const NXDouble &scan) const {
  std::vector<DateAndTime> times;
  std::vector<double> durations = getDurations(scan);
  DateAndTime time = m_startTime;
  times.emplace_back(time);
  size_t timeIndex = 1;
  while (timeIndex < m_numberScanPoints) {
    time += durations[timeIndex - 1];
    times.emplace_back(time);
    ++timeIndex;
  }
  return times;
}

/**
 * Resolves the scan type
 */
void LoadILLDiffraction::resolveScanType() {
  ScanType result = NoScan;
  if (m_instName == "D2B") {
    result = DetectorScan;
  } else {
    if (m_numberScanPoints != 1) {
      for (const auto &scanVar : m_scanVar) {
        if (scanVar.scanned == 1) {
          result = OtherScan;
          if (scanVar.name == "2theta") {
            result = DetectorScan;
            break;
          }
        }
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
      // Some amount of pixels are dead on at right end, these have to be
      // subtracted
      // correspondingly dependent on the resolution mode
      m_resolutionMode = m_numberDetectorsRead / D20_NUMBER_PIXELS;
      size_t activePixels = D20_NUMBER_PIXELS - 2 * D20_NUMBER_DEAD_PIXELS;
      m_numberDetectorsActual = m_resolutionMode * activePixels;

      if (m_resolutionMode > 3 || m_resolutionMode < 1) {
        throw std::runtime_error("Unknown resolution mode for instrument " + m_instName);
      }
      if (m_resolutionMode == 1) {
        m_instName += "_lr";
      } else if (m_resolutionMode == 3) {
        m_instName += "_hr";
      }
    }
    g_log.debug() << "Instrument name is " << m_instName << " and has " << m_numberDetectorsActual
                  << " actual detectors.\n";
  }
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
    twoTheta0Actual += m_offsetTheta;
  }
  Quat rotation(twoTheta0Actual, V3D(0, 1, 0));
  g_log.debug() << "Setting 2theta0 to " << twoTheta0Actual;
  auto &componentInfo = m_outWorkspace->mutableComponentInfo();
  const auto componentIndex = componentInfo.indexOf(component->getComponentID());
  componentInfo.setRotation(componentIndex, rotation);
}

/**
 * Makes up the full path of the relevant IDF dependent on resolution mode
 * @param instName : the name of the instrument (including the resolution mode
 * suffix)
 * @return : the full path to the corresponding IDF
 */
std::string LoadILLDiffraction::getInstrumentFilePath(const std::string &instName) const {

  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(instName + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

/** Adds some sample logs needed later by reduction
 *  @throw runtime_error If wavelength or Monochromator.ei metadata is missing from the file
 */
void LoadILLDiffraction::setSampleLogs() {
  Run &run = m_outWorkspace->mutableRun();
  std::string scanTypeStr = "NoScan";
  if (m_scanType == DetectorScan) {
    scanTypeStr = "DetectorScan";
  } else if (m_scanType == OtherScan) {
    scanTypeStr = "OtherScan";
  }
  run.addLogData(new PropertyWithValue<std::string>("ScanType", std::move(scanTypeStr)));
  run.addLogData(new PropertyWithValue<double>("PixelSize", D20_PIXEL_SIZE / static_cast<double>(m_resolutionMode)));
  std::string resModeStr = "Nominal";
  if (m_resolutionMode == 1) {
    resModeStr = "Low";
  } else if (m_resolutionMode == 3) {
    resModeStr = "High";
  }
  run.addLogData(new PropertyWithValue<std::string>("ResolutionMode", std::move(resModeStr)));
  if (m_scanType != NoScan) {
    run.addLogData(new PropertyWithValue<int>("ScanSteps", static_cast<int>(m_numberScanPoints)));
  }
  double eFixed;
  if (run.hasProperty("wavelength")) {
    double lambda = run.getLogAsSingleValue("wavelength");
    eFixed = WAVE_TO_E / (lambda * lambda);
  } else if (run.hasProperty("Monochromator.ei")) { // D4C, wavelength is not specified and Ei is provided directly
    eFixed = run.getPropertyValueAsType<double>("Monochromator.ei");
  } else {
    throw std::runtime_error("Neither wavelength nor Monochromator.ei are not specified in the loaded file.");
  }
  run.addLogData(std::make_unique<Kernel::PropertyWithValue<double>>(PropertyWithValue<double>("Ei", eFixed)), true);
  run.addLogData(new PropertyWithValue<size_t>("NumberOfDetectors", m_numberDetectorsActual));
  if (m_pixelHeight != 0.) {
    run.addLogData(new PropertyWithValue<double>("PixelHeight", m_pixelHeight));
  }
  if (m_maxHeight != 0.) {
    run.addLogData(new PropertyWithValue<double>("MaxHeight", m_maxHeight));
  }
}

/**
 * Computes the 2theta offset of the decoder for D20
 */
void LoadILLDiffraction::computeThetaOffset() {
  m_offsetTheta = static_cast<double>(D20_NUMBER_DEAD_PIXELS) * D20_PIXEL_SIZE -
                  D20_PIXEL_SIZE / (static_cast<double>(m_resolutionMode) * 2);
}

/**
 * Converts the spectrum axis to 2theta and transposes the workspace.
 */
void LoadILLDiffraction::convertAxisAndTranspose() {
  auto extractor = createChildAlgorithm("ExtractSpectra");
  extractor->setProperty("InputWorkspace", m_outWorkspace);
  extractor->setProperty("StartWorkspaceIndex", 1);
  extractor->setProperty("OutputWorkspace", "__unused");
  extractor->execute();
  API::MatrixWorkspace_sptr det = extractor->getProperty("OutputWorkspace");
  auto converter = createChildAlgorithm("ConvertSpectrumAxis");
  converter->setProperty("InputWorkspace", det);
  converter->setProperty("OutputWorkspace", "__unused");
  converter->setProperty("Target", "SignedTheta");
  converter->execute();
  API::MatrixWorkspace_sptr converted = converter->getProperty("OutputWorkspace");
  auto transposer = createChildAlgorithm("Transpose");
  transposer->setProperty("InputWorkspace", converted);
  transposer->setProperty("OutputWorkspace", "__unused");
  transposer->execute();
  API::MatrixWorkspace_sptr transposed = transposer->getProperty("OutputWorkspace");
  m_outWorkspace = transposed;
}

} // namespace Mantid::DataHandling
