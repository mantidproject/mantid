// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLReflectometry.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"

namespace {

/// Convert wavelength to TOF
double wavelengthToTOF(const double lambda, const double l1, const double l2) {
  return Mantid::Kernel::UnitConversion::run("Wavelength", "TOF", lambda, l1, l2, 0.,
                                             Mantid::Kernel::DeltaEMode::Elastic, 0.);
}

/** Convert degrees to radians.
 *  @param x an angle in degrees
 *  @return the angle in radians
 */
constexpr double degToRad(const double x) { return x * M_PI / 180.; }

/** Convert radians to degrees.
 *  @param x an angle in radians
 *  @return the angle in degrees
 */
constexpr double radToDeg(const double x) { return x * 180. / M_PI; }

/** Convert millimeters to meters.
 *  @param x a distance in millimeters
 *  @return the distance in meters
 */
constexpr double mmToMeter(const double x) { return x * 1.e-3; }

/** Strip monitors from the beginning and end of a workspace.
 *  @param ws a workspace to work on
 *  @return begin and end ws indices for non-monitor histograms
 */
std::pair<int, int> fitIntegrationWSIndexRange(const Mantid::API::MatrixWorkspace &ws) {
  const size_t nHisto = ws.getNumberHistograms();
  int begin = 0;
  const auto &spectrumInfo = ws.spectrumInfo();
  for (size_t i = 0; i < nHisto; ++i) {
    if (!spectrumInfo.isMonitor(i)) {
      break;
    }
    ++begin;
  }
  int end = static_cast<int>(nHisto) - 1;
  for (ptrdiff_t i = static_cast<ptrdiff_t>(nHisto) - 1; i != 0; --i) {
    if (!spectrumInfo.isMonitor(i)) {
      break;
    }
    --end;
  }
  return std::pair<int, int>{begin, end};
}

/** Fill the X values of the first histogram of ws with values 0, 1, 2,...
 *  @param ws a workspace to modify
 */
void rebinIntegralWorkspace(Mantid::API::MatrixWorkspace &ws) {
  auto &xs = ws.mutableX(0);
  std::iota(xs.begin(), xs.end(), 0.0);
}

/// Enumerations to define the rotation plane of the detector.
enum class RotationPlane { horizontal, vertical };

/** Calculate the detector position from given parameters.
 *  @param plane rotation plane of the detector
 *  @param distance sample to detector centre distance in meters
 *  @param angle an angle between the Z axis and the detector in degrees
 *  @return a vector pointing to the new detector centre
 */
Mantid::Kernel::V3D detectorPosition(const RotationPlane plane, const double distance, const double angle) {
  const double a = degToRad(angle);
  double x = 0, y = 0, z = 0;
  switch (plane) {
  case RotationPlane::horizontal:
    x = distance * std::sin(a);
    z = distance * std::cos(a);
    break;
  case RotationPlane::vertical:
    y = distance * std::sin(a);
    z = distance * std::cos(a);
    break;
  }
  return Mantid::Kernel::V3D(x, y, z);
}

/** Calculates the detector rotation such that it faces the origin.
 *  @param plane rotation plane of the detectorPosition
 *  @param angle an angle between the Z axis and the detector in degrees
 *  @return the calculated rotation transformation
 */
Mantid::Kernel::Quat detectorFaceRotation(const RotationPlane plane, const double angle) {
  const Mantid::Kernel::V3D axis = [plane]() {
    double x = 0, y = 0;
    switch (plane) {
    case RotationPlane::horizontal:
      y = 1;
      break;
    case RotationPlane::vertical:
      x = -1;
      break;
    }
    return Mantid::Kernel::V3D(x, y, 0);
  }();
  return Mantid::Kernel::Quat(angle, axis);
}
} // anonymous namespace

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;
using Mantid::Types::Core::DateAndTime;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLReflectometry)

/**
 * Return the confidence with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLReflectometry::confidence(Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the ILL
  if ((descriptor.pathExists("/entry0/wavelength") || // ILL D17
       descriptor.pathExists("/entry0/theta"))        // ILL FIGARO
      && descriptor.pathExists("/entry0/experiment_identifier") && descriptor.pathExists("/entry0/mode") &&
      (descriptor.pathExists("/entry0/instrument/VirtualChopper") || // ILL D17
       descriptor.pathExists("/entry0/instrument/Theta"))            // ILL FIGARO
  )
    return 80;
  else
    return 0;
}

/// Initialize the algorithm's properties.
void LoadILLReflectometry::init() {
  declareProperty(
      std::make_unique<FileProperty>("Filename", std::string(), FileProperty::Load, ".nxs", Direction::Input),
      "Name of the Nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", std::string(), Direction::Output),
                  "Name of the output workspace");
  declareProperty("ForegroundPeakCentre", EMPTY_DBL(),
                  "Foreground peak position in fractional workspace "
                  "index (if not given the peak is searched for and fitted).");
  declareProperty("DetectorCentreFractionalIndex", 127.5,
                  "The fractional workspace index of the geometric centre of "
                  "the detector at incident beam axis (127.5 for D17 and Figaro).");
  const std::vector<std::string> measurements({"DirectBeam", "ReflectedBeam"});
  declareProperty("Measurement", "DirectBeam", std::make_unique<StringListValidator>(measurements),
                  "Load as direct or reflected beam.");
  declareProperty("BraggAngle", EMPTY_DBL(), "The bragg angle necessary for reflected beam.");
  declareProperty("FitStartWorkspaceIndex", 0, std::make_unique<BoundedValidator<int>>(0, 255),
                  "Start workspace index used for peak fitting.");
  declareProperty("FitEndWorkspaceIndex", 255, std::make_unique<BoundedValidator<int>>(0, 255),
                  "End workspace index used for peak fitting.");
  declareProperty("FitRangeLower", -1., "Minimum wavelength used for peak fitting.");
  declareProperty("FitRangeUpper", -1., "Maximum wavelength used for peak fitting.");
  const std::vector<std::string> availableUnits{"Wavelength", "TimeOfFlight"};
  declareProperty("XUnit", "Wavelength", std::make_shared<StringListValidator>(availableUnits),
                  "X unit of the OutputWorkspace");
  declareProperty(std::make_unique<PropertyManagerProperty>("LogsToReplace", Direction::Input),
                  "A dictionary of key-pair values for logs to be replaced.");
}

/// Execute the algorithm.
void LoadILLReflectometry::exec() {
  NeXus::NXRoot root(getPropertyValue("Filename"));
  NXEntry firstEntry{root.openFirstEntry()};
  initNames(firstEntry);
  sampleAngle(firstEntry);
  std::vector<std::string> monitorNames{getMonitorNames()};
  loadDataDetails(firstEntry);
  initWorkspace(monitorNames);
  LoadHelper::loadEmptyInstrument(m_localWorkspace, m_instrument == Supported::D17 ? "D17" : "FIGARO");
  loadNexusEntriesIntoProperties();
  loadData(firstEntry, monitorNames, getXValues());
  firstEntry.close();
  root.close();
  initPixelWidth();
  sampleHorizontalOffset();
  placeSource();
  placeDetector();
  placeSlits();
  convertTofToWavelength();
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Init names of sample logs based on instrument specific NeXus file
 * entries
 *
 * @param entry :: the NeXus file entry
 */
void LoadILLReflectometry::initNames(const NeXus::NXEntry &entry) {
  std::string instrumentNamePath = LoadHelper::findInstrumentNexusPath(entry);
  std::string instrumentName = entry.getString(instrumentNamePath.append("/name"));
  if (instrumentName.empty())
    throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
  boost::to_lower(instrumentName);
  if (instrumentName == "d17") {
    m_instrument = Supported::D17;
  } else if (instrumentName == "figaro") {
    m_instrument = Supported::FIGARO;
  } else {
    std::ostringstream str;
    str << "Unsupported instrument: " << instrumentName << '.';
    throw std::runtime_error(str.str());
  }
  g_log.debug() << "Instrument name: " << instrumentName << '\n';
  if (m_instrument == Supported::D17) {
    m_offsetFrom = "VirtualChopper";
    m_chopper1Name = "Chopper1";
    m_chopper2Name = "Chopper2";
  } else if (m_instrument == Supported::FIGARO) {
    m_sampleAngleName = "CollAngle.actual_coll_angle";
    m_offsetFrom = "CollAngle";
    // FIGARO: find out which of the four choppers are used
    NXInt firstChopper = entry.openNXInt("instrument/ChopperSetting/firstChopper");
    firstChopper.load();
    NXInt secondChopper = entry.openNXInt("instrument/ChopperSetting/secondChopper");
    secondChopper.load();
    m_chopper1Name = "chopper" + std::to_string(firstChopper[0]);
    m_chopper2Name = "chopper" + std::to_string(secondChopper[0]);
  }
  // get acquisition mode
  NXInt acqMode = entry.openNXInt("acquisition_mode");
  acqMode.load();
  m_acqMode = acqMode[0];
  m_acqMode ? g_log.debug("TOF mode") : g_log.debug("Monochromatic Mode");
}

/** Call child algorithm ConvertUnits for conversion from TOF to wavelength
 * Note that DAN calibration is done in preprocess, since it needs information
 * also from the direct beam so converting to wavelength in the loader will not
 * be accurate
 */
void LoadILLReflectometry::convertTofToWavelength() {
  if (m_acqMode && (getPropertyValue("XUnit") == "Wavelength")) {
    auto convertToWavelength = createChildAlgorithm("ConvertUnits", -1, -1, true);
    convertToWavelength->initialize();
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_localWorkspace);
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", m_localWorkspace);
    convertToWavelength->setPropertyValue("Target", "Wavelength");
    convertToWavelength->executeAsChildAlg();
  }
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param monitorNames :: Monitors data already loaded
 */
void LoadILLReflectometry::initWorkspace(const std::vector<std::string> &monitorNames) {

  g_log.debug() << "Number of monitors: " << monitorNames.size() << '\n';
  for (size_t i = 0; i < monitorNames.size(); ++i) {
    if (monitorNames[i].size() != m_numberOfChannels)
      g_log.debug() << "Data size of monitor ID " << i << " is " << monitorNames[i].size() << '\n';
  }
  // create the workspace
  m_localWorkspace = DataObjects::create<DataObjects::Workspace2D>(m_numberOfHistograms + monitorNames.size(),
                                                                   HistogramData::BinEdges(m_numberOfChannels + 1));

  if (m_acqMode)
    m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnitLabel("Counts");

  // the start time is needed in the workspace when loading the parameter file
  m_localWorkspace->mutableRun().addProperty<std::string>("start_time", m_startTime.toISO8601String());
}

/**
 * Load Data details (number of tubes, channels, etc)
 *
 * @param entry First entry of nexus file
 */
void LoadILLReflectometry::loadDataDetails(const NeXus::NXEntry &entry) {
  // PSD data D17 256 x 1 x 1000
  // PSD data FIGARO 1 x 256 x 1000
  m_startTime = DateAndTime(LoadHelper::dateTimeInIsoFormat(entry.getString("start_time")));
  if (m_acqMode) {
    NXFloat timeOfFlight = entry.openNXFloat("instrument/PSD/time_of_flight");
    timeOfFlight.load();
    m_channelWidth = static_cast<double>(timeOfFlight[0]);
    m_numberOfChannels = size_t(timeOfFlight[1]);
    m_tofDelay = timeOfFlight[2];
  } else { // monochromatic mode
    m_numberOfChannels = 1;
  }
  NXInt nChannels = entry.openNXInt("instrument/PSD/detsize");
  nChannels.load();
  m_numberOfHistograms = nChannels[0];
}

/**
 * @brief LoadILLReflectometry::doubleFromRun
 * Returns a sample log a single double
 * @param entryName : the name of the log
 * @return the value as double
 * @throws runtime_error : if the log does not exist
 */
double LoadILLReflectometry::doubleFromRun(const std::string &entryName) const {
  if (m_localWorkspace->run().hasProperty(entryName)) {
    return m_localWorkspace->run().getPropertyValueAsType<double>(entryName);
  } else {
    throw std::runtime_error("The log with the given name does not exist " + entryName);
  }
}

/**
 * Load monitor data
 *
 * @return :: A std::vector of vectors of monitors containing monitor values
 */
std::vector<std::string> LoadILLReflectometry::getMonitorNames() {
  // vector of paths to monitor data
  const std::vector<std::string> monitors{"monitor1/data", "monitor2/data"};
  return monitors;
}

/**
 * Determine x values (unit time-of-flight)
 *
 * @return :: vector holding the x values
 */
std::vector<double> LoadILLReflectometry::getXValues() {
  const auto &instrument = m_localWorkspace->getInstrument();
  const auto &run = m_localWorkspace->run();
  std::vector<double> xVals;             // no initialisation
  xVals.reserve(m_numberOfChannels + 1); // reserve memory
  if (m_acqMode) {
    if (m_instrument == Supported::FIGARO) {
      if (run.hasProperty("Distance.edelay_delay"))
        m_tofDelay += doubleFromRun("Distance.edelay_delay");
      else if (run.hasProperty("Theta.edelay_delay"))
        m_tofDelay += doubleFromRun("Theta.edelay_delay");
      else if (run.hasProperty("MainParameters.edelay_delay")) {
        m_tofDelay += doubleFromRun("MainParameters.edelay_delay");
      } else {
        g_log.warning() << "Unable to find edelay_delay from the file\n";
      }
    }
    g_log.debug() << "TOF delay: " << m_tofDelay << '\n';
    std::string chopper{"Chopper"};
    double chop1Speed{0.0}, chop1Phase{0.0}, chop2Speed{0.0}, chop2Phase{0.0};
    if (m_instrument == Supported::D17) {
      const auto duration = doubleFromRun("duration");
      std::string chop1SpeedName, chop1PhaseName, chop2SpeedName, chop2PhaseName;
      if (duration > 30.0) {
        chop1SpeedName = instrument->getStringParameter("chopper1_speed")[0];
        chop1PhaseName = instrument->getStringParameter("chopper1_phase")[0];
        chop2SpeedName = instrument->getStringParameter("chopper2_speed")[0];
        chop2PhaseName = instrument->getStringParameter("chopper2_phase")[0];
      } else {
        chop1SpeedName = instrument->getStringParameter("chopper1_speed_alt")[0];
        chop1PhaseName = instrument->getStringParameter("chopper1_phase_alt")[0];
        chop2SpeedName = instrument->getStringParameter("chopper2_speed_alt")[0];
        chop2PhaseName = instrument->getStringParameter("chopper2_phase_alt")[0];
      }
      chop1Speed = doubleFromRun(chop1SpeedName);
      chop1Phase = doubleFromRun(chop1PhaseName);
      chop2Speed = doubleFromRun(chop2SpeedName);
      chop2Phase = doubleFromRun(chop2PhaseName);
      if (chop1Phase > 360.) {
        // Pre-2018 D17 files which have chopper 1 phase and chopper 2 speed
        // swapped.
        std::swap(chop1Phase, chop2Speed);
      }
    } else if (m_instrument == Supported::FIGARO) {
      chop1Phase = doubleFromRun(m_chopper1Name + ".phase");
      // Chopper 1 phase on FIGARO is set to an arbitrary value (999.9)
      if (chop1Phase > 360.0)
        chop1Phase = 0.0;
    }
    double POFF;
    if (run.hasProperty(m_offsetFrom + ".poff")) {
      POFF = doubleFromRun(m_offsetFrom + ".poff");
    } else if (run.hasProperty(m_offsetFrom + ".pickup_offset")) {
      POFF = doubleFromRun(m_offsetFrom + ".pickup_offset");
    } else {
      throw std::runtime_error("Unable to find chopper pickup offset");
    }
    double openOffset;
    if (run.hasProperty(m_offsetFrom + ".open_offset")) {
      openOffset = doubleFromRun(m_offsetFrom + ".open_offset");
    } else if (run.hasProperty(m_offsetFrom + ".openOffset")) {
      openOffset = doubleFromRun(m_offsetFrom + ".openOffset");
    } else {
      throw std::runtime_error("Unable to find chopper open offset");
    }
    if (m_instrument == Supported::D17 && chop1Speed != 0.0 && chop2Speed != 0.0 && chop2Phase != 0.0) {
      // virtual chopper entries are valid
      chopper = "Virtual chopper";
    } else {
      // use chopper values
      chop1Speed = doubleFromRun(m_chopper1Name + ".rotation_speed");
      chop2Speed = doubleFromRun(m_chopper2Name + ".rotation_speed");
      chop2Phase = doubleFromRun(m_chopper2Name + ".phase");
    }
    // logging
    g_log.debug() << "Poff: " << POFF << '\n';
    g_log.debug() << "Open offset: " << openOffset << '\n';
    g_log.debug() << "Chopper 1 phase: " << chop1Phase << '\n';
    g_log.debug() << chopper << " 1 speed: " << chop1Speed << '\n';
    g_log.debug() << chopper << " 2 phase: " << chop2Phase << '\n';
    g_log.debug() << chopper << " 2 speed: " << chop2Speed << '\n';

    if (chop1Speed <= 0.0) {
      g_log.error() << "First chopper velocity " << chop1Speed << ". Check you NeXus file.\n";
    }

    const double chopWindow = instrument->getNumberParameter("chopper_window_opening")[0];
    m_localWorkspace->mutableRun().addProperty("ChopperWindow", chopWindow, "degree", true);
    g_log.debug() << "Chopper Opening Window [degrees]" << chopWindow << '\n';

    const double t_TOF2 = m_tofDelay - 1.e+6 * 60.0 * (POFF - chopWindow + chop2Phase - chop1Phase + openOffset) /
                                           (2.0 * 360 * chop1Speed);
    g_log.debug() << "t_TOF2: " << t_TOF2 << '\n';
    // compute tof values
    for (int channelIndex = 0; channelIndex < static_cast<int>(m_numberOfChannels) + 1; ++channelIndex) {
      const double t_TOF1 = channelIndex * m_channelWidth;
      xVals.emplace_back(t_TOF1 + t_TOF2);
    }
  } else {
    g_log.debug("Time channel index for axis description \n");
    for (size_t t = 0; t <= m_numberOfChannels; ++t)
      xVals.emplace_back(static_cast<double>(t));
  }

  return xVals;
}

/**
 * Load data from nexus file
 *
 * @param entry :: The Nexus file entry
 * @param monitorNames :: Monitors data already loaded
 * @param xVals :: X values
 */
void LoadILLReflectometry::loadData(const NeXus::NXEntry &entry, const std::vector<std::string> &monitorNames,
                                    const std::vector<double> &xVals) {
  auto data = LoadHelper::getIntDataset(entry, "data");
  data.load();
  const int nb_monitors = static_cast<int>(monitorNames.size());
  Progress progress(this, 0, 1, m_numberOfHistograms + nb_monitors);
  if (!xVals.empty()) {
    // first, load data
    LoadHelper::fillStaticWorkspace(m_localWorkspace, data, xVals, 0);
    progress.report();
    // then, the monitor data
    for (auto im = 0; im < nb_monitors; ++im) {
      const std::string monitorDataSetName("monitor" + std::to_string(im + 1) + "/data");
      auto monitorData = LoadHelper::getIntDataset(entry, monitorDataSetName);
      monitorData.load();
      LoadHelper::fillStaticWorkspace(m_localWorkspace, monitorData, xVals,
                                      static_cast<int>(m_numberOfHistograms) + im);
      progress.report();
    }
  }
}

/**
 * Use the LoadHelper utility to load most of the nexus entries into workspace
 * sample log properties
 */
void LoadILLReflectometry::loadNexusEntriesIntoProperties() {
  const std::string filename{getPropertyValue("Filename")};
  NXhandle nxfileID;
  NXstatus stat = NXopen(filename.c_str(), NXACC_READ, &nxfileID);
  if (stat == NX_ERROR)
    throw Kernel::Exception::FileError("Unable to open File:", filename);
  API::Run &runDetails = m_localWorkspace->mutableRun();
  LoadHelper::addNexusFieldsToWsRun(nxfileID, runDetails);
  NXclose(&nxfileID);
  if (m_instrument == Supported::FIGARO) {
    auto const bgs3 = m_localWorkspace->mutableRun().getLogAsSingleValue("BGS3.value");
    // log data below should be a boolean, but boolean is broken in NeXus so it cannot be saved properly
    // when exporting data.
    m_localWorkspace->mutableRun().addLogData(
        new Kernel::PropertyWithValue<int>("refdown", static_cast<int>(bgs3 > 45)));
  }
  const PropertyManager_const_sptr logsToReplace = getProperty("LogsToReplace");
  if (logsToReplace != nullptr && logsToReplace->propertyCount() > 0) {
    for (auto *prop : logsToReplace->getProperties()) {
      if (prop->type() == "number") { // logs are either treated as numbers and cast to double or are saved as strings
        runDetails.addProperty(prop->name(), std::stod(prop->value()), true);
      } else
        runDetails.addProperty(prop->name(), prop->value(), true);
    }
  }
}

/**
 * Gaussian fit to determine peak position if no user position given.
 *
 * @return :: detector position of the peak: Gaussian fit and position
 * of the maximum (serves as start value for the optimization)
 */
double LoadILLReflectometry::reflectometryPeak() {
  if (!isDefault("ForegroundPeakCentre")) {
    return getProperty("ForegroundPeakCentre");
  }
  const auto autoIndices = fitIntegrationWSIndexRange(*m_localWorkspace);
  auto startIndex = autoIndices.first;
  auto endIndex = autoIndices.second;
  if (!isDefault("FitStartWorkspaceIndex")) {
    startIndex = getProperty("FitStartWorkspaceIndex");
  }
  if (!isDefault("FitEndWorkspaceIndex")) {
    endIndex = getProperty("FitEndWorkspaceIndex");
  }
  auto integration = createChildAlgorithm("Integration");
  integration->initialize();
  integration->setProperty("InputWorkspace", m_localWorkspace);
  integration->setProperty("OutputWorkspace", "__unused_for_child");
  integration->setProperty("StartWorkspaceIndex", startIndex);
  integration->setProperty("EndWorkspaceIndex", endIndex);
  if (!isDefault("FitRangeLower")) {
    integration->setProperty("RangeLower",
                             wavelengthToTOF(getProperty("FitRangeLower"), m_sourceDistance, m_detectorDistance));
  }
  if (!isDefault("FitRangeUpper")) {
    integration->setProperty("RangeUpper",
                             wavelengthToTOF(getProperty("FitRangeUpper"), m_sourceDistance, m_detectorDistance));
  }
  integration->execute();
  MatrixWorkspace_sptr integralWS = integration->getProperty("OutputWorkspace");
  auto transpose = createChildAlgorithm("Transpose");
  transpose->initialize();
  transpose->setProperty("InputWorkspace", integralWS);
  transpose->setProperty("OutputWorkspace", "__unused_for_child");
  transpose->execute();
  integralWS = transpose->getProperty("OutputWorkspace");
  rebinIntegralWorkspace(*integralWS);
  // determine initial height: maximum value
  const auto maxValueIt = std::max_element(integralWS->y(0).cbegin(), integralWS->y(0).cend());
  const double height = *maxValueIt;
  // determine initial centre: index of the maximum value
  const size_t maxIndex = std::distance(integralWS->y(0).cbegin(), maxValueIt);
  const auto centreByMax = static_cast<double>(maxIndex);
  const auto &ys = integralWS->y(0);
  auto lessThanHalfMax = [height](const double x) { return x < 0.5 * height; };
  using IterType = HistogramData::HistogramY::const_iterator;
  std::reverse_iterator<IterType> revMaxValueIt{maxValueIt};
  auto revMinFwhmIt = std::find_if(revMaxValueIt, ys.crend(), lessThanHalfMax);
  auto maxFwhmIt = std::find_if(maxValueIt, ys.cend(), lessThanHalfMax);
  std::reverse_iterator<IterType> revMaxFwhmIt{maxFwhmIt};
  if (revMinFwhmIt == ys.crend() || maxFwhmIt == ys.cend()) {
    return centreByMax + startIndex;
  }
  const auto fwhm = static_cast<double>(std::distance(revMaxFwhmIt, revMinFwhmIt) + 1);
  // generate Gaussian
  auto func = API::FunctionFactory::Instance().createFunction("CompositeFunction");
  auto sum = std::dynamic_pointer_cast<API::CompositeFunction>(func);
  func = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto gaussian = std::dynamic_pointer_cast<API::IPeakFunction>(func);
  gaussian->setHeight(height);
  gaussian->setCentre(centreByMax);
  gaussian->setFwhm(fwhm);
  sum->addFunction(gaussian);
  func = API::FunctionFactory::Instance().createFunction("LinearBackground");
  func->setParameter("A0", 0.);
  func->setParameter("A1", 0.);
  sum->addFunction(func);
  // call Fit child algorithm
  auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", std::dynamic_pointer_cast<API::IFunction>(sum));
  fit->setProperty("InputWorkspace", integralWS);
  fit->setProperty("StartX", centreByMax - 3 * fwhm);
  fit->setProperty("EndX", centreByMax + 3 * fwhm);
  fit->execute();
  const std::string fitStatus = fit->getProperty("OutputStatus");
  if (fitStatus != "success") {
    g_log.warning("Fit not successful, using position of max value.\n");
    return centreByMax + startIndex;
  }
  const auto centre = gaussian->centre();
  return centre + startIndex;
}

/** Compute the detector rotation angle around origin
 *  @return a rotation angle
 */
double LoadILLReflectometry::detectorRotation() {
  const double peakCentre = reflectometryPeak();
  m_localWorkspace->mutableRun().addProperty("reduction.line_position", peakCentre, true);
  const double detectorCentre = getProperty("DetectorCentreFractionalIndex");
  const std::string measurement = getPropertyValue("Measurement");
  double two_theta = offsetAngle(peakCentre, detectorCentre, m_detectorDistance);
  if (measurement == "ReflectedBeam") {
    if (isDefault("BraggAngle")) {
      if (m_sampleAngle == 0.) {
        g_log.warning("Sample angle is either 0 or doesn't exist in the file. "
                      "Please specify BraggAngle manually for reflected beams.");
      }
    }
    two_theta += 2 * (isDefault("BraggAngle") ? m_sampleAngle : getProperty("BraggAngle"));
  }
  return two_theta;
}

/** Sets the sample angle (i.e. bragg angle) [degrees]
 * Used when measurement type is reflected beam (otherwise must be zero)
 * Note that DAN calibration needs information also from the corresponding
 * direct beam, hence it cannot be done in the loader, but it is done in
 * preprocessing algorithm. However loader should still support loading
 * reflected beams standalone, hence sample angle is the only option if
 * BraggAngle is not manually specified.
 *
 * @param entry :: The Nexus file entry
 */
void LoadILLReflectometry::sampleAngle(const NeXus::NXEntry &entry) {
  std::string entryName;
  if (m_instrument == Supported::D17) {
    if (entry.isValid("instrument/SAN/value")) {
      entryName = "instrument/SAN/value";
    } else if (entry.isValid("instrument/san/value")) {
      entryName = "instrument/san/value";
    }
  } else {
    if (entry.isValid("instrument/Theta/wanted_theta")) {
      entryName = "instrument/Theta/wanted_theta";
    }
  }
  if (!entryName.empty()) {
    NXFloat angle = entry.openNXFloat(entryName);
    angle.load();
    m_sampleAngle = angle[0];
  }
}

/// Initialize m_pixelWidth from the IDF as the step of rectangular detector
void LoadILLReflectometry::initPixelWidth() {
  const auto &instrument = m_localWorkspace->getInstrument();
  const auto &detectorPanels = instrument->getAllComponentsWithName("detector");
  if (detectorPanels.size() != 1) {
    throw std::runtime_error("IDF should have a single 'detector' component.");
  }
  const auto &detector = std::dynamic_pointer_cast<const Geometry::RectangularDetector>(detectorPanels.front());
  if (m_instrument == Supported::D17) {
    m_pixelWidth = std::abs(detector->xstep());
  } else {
    m_pixelWidth = std::abs(detector->ystep());
  }
}

/// Update detector position according to data file
void LoadILLReflectometry::placeDetector() {
  m_detectorDistance = sampleDetectorDistance();
  m_localWorkspace->mutableRun().addProperty<double>("L2", m_detectorDistance, true);
  const auto detectorRotationAngle = detectorRotation();
  const std::string componentName = "detector";
  const RotationPlane rotPlane = m_instrument == Supported::D17 ? RotationPlane::horizontal : RotationPlane::vertical;
  const auto newpos = detectorPosition(rotPlane, m_detectorDistance, detectorRotationAngle);
  LoadHelper::moveComponent(m_localWorkspace, componentName, newpos);
  // apply a local rotation to stay perpendicular to the beam
  const auto rotation = detectorFaceRotation(rotPlane, detectorRotationAngle);
  LoadHelper::rotateComponent(m_localWorkspace, componentName, rotation);
}

/// Update the slit positions.
void LoadILLReflectometry::placeSlits() {
  double slit1ToSample{0.0};
  double slit2ToSample{0.0};
  const auto &run = m_localWorkspace->run();
  if (m_instrument == Supported::FIGARO) {
    const double deflectionAngle = doubleFromRun(m_sampleAngleName);
    const double offset = m_sampleZOffset / std::cos(degToRad(deflectionAngle));
    if (run.hasProperty("Distance.S2_Sample")) {
      slit1ToSample = mmToMeter(doubleFromRun("Distance.S2_Sample"));
    } else {
      throw std::runtime_error("Unable to find slit 2 to sample distance");
    }
    if (run.hasProperty("Distance.S3_Sample")) {
      slit2ToSample = mmToMeter(doubleFromRun("Distance.S3_Sample"));
    } else {
      throw std::runtime_error("Unable to find slit 3 to sample distance");
    }
    slit2ToSample += offset;
    slit1ToSample += offset;
  } else {
    if (run.hasProperty("Distance.S2toSample")) {
      slit1ToSample = mmToMeter(doubleFromRun("Distance.S2toSample"));
    } else if (run.hasProperty("Distance.S2_Sample")) {
      slit1ToSample = mmToMeter(doubleFromRun("Distance.S2_Sample"));
    } else {
      throw std::runtime_error("Unable to find slit 2 to sample distance");
    }
    if (run.hasProperty("Distance.S3toSample")) {
      slit2ToSample = mmToMeter(doubleFromRun("Distance.S3toSample"));
    } else if (run.hasProperty("Distance.S3_Sample")) {
      slit2ToSample = mmToMeter(doubleFromRun("Distance.S3_Sample"));
    } else {
      throw std::runtime_error("Unable to find slit 3 to sample distance");
    }
  }
  V3D pos{0.0, 0.0, -slit1ToSample};
  LoadHelper::moveComponent(m_localWorkspace, "slit2", pos);
  pos = {0.0, 0.0, -slit2ToSample};
  LoadHelper::moveComponent(m_localWorkspace, "slit3", pos);
}

/// Update source position.
void LoadILLReflectometry::placeSource() {
  m_sourceDistance = sourceSampleDistance();
  const std::string source = "chopper1";
  const V3D newPos{0.0, 0.0, -m_sourceDistance};
  LoadHelper::moveComponent(m_localWorkspace, source, newPos);
}

/// Return the incident neutron deflection angle.
double LoadILLReflectometry::collimationAngle() const {
  return m_instrument == Supported::FIGARO ? doubleFromRun(m_sampleAngleName) : 0.;
}

/** Calculate the offset angle between detector center and peak.
 *  @param peakCentre peak centre in pixels.
 *  @param detectorCentre detector centre in pixels.
 *  @param detectorDistance detector-sample distance in meters.
 *  @return the offset angle.
 */
double LoadILLReflectometry::offsetAngle(const double peakCentre, const double detectorCentre,
                                         const double detectorDistance) const {
  const double offsetWidth = (detectorCentre - peakCentre) * m_pixelWidth;
  // Sign depends on the definition of detector angle and which way
  // spectrum numbers increase. Negative convention is used for D17 and positive for FIGARO.
  auto const sign = m_instrument == Supported::FIGARO ? 1 : -1;
  return sign * radToDeg(std::atan2(offsetWidth, detectorDistance));
}

/** Return the sample to detector distance for the current instrument.
 *  @return the distance in meters
 */
double LoadILLReflectometry::sampleDetectorDistance() const {
  std::string distanceEntry;
  if (m_instrument == Supported::D17) {
    distanceEntry = "det.value";
  } else {
    distanceEntry = "Distance.Sample_CenterOfDetector_distance";
  }
  return mmToMeter(doubleFromRun(distanceEntry));
}

/// Return the horizontal offset along the z axis.
void LoadILLReflectometry::sampleHorizontalOffset() {
  if (m_instrument == Supported::FIGARO) {
    std::string offsetEntry;
    const auto &run = m_localWorkspace->run();
    if (run.hasProperty("Theta.sampleHorizontalOffset"))
      offsetEntry = "Theta.sampleHorizontalOffset";
    else if (run.hasProperty("Distance.sampleHorizontalOffset")) {
      offsetEntry = "Distance.sampleHorizontalOffset";
    } else if (run.hasProperty("Distance.sample_changer_horizontal_offset")) {
      offsetEntry = "Distance.sample_changer_horizontal_offset";
    } else if (run.hasProperty("Theta.sample_horizontal_offset")) {
      offsetEntry = "Theta.sample_horizontal_offset";
    } else {
      throw std::runtime_error("Unable to find sample horizontal offset in the file");
    }
    m_sampleZOffset = mmToMeter(doubleFromRun(offsetEntry));
  }
}

/** Return the source to sample distance for the current instrument.
 *  @return the source to sample distance in meters
 */
double LoadILLReflectometry::sourceSampleDistance() const {
  const auto &run = m_localWorkspace->run();
  if (m_instrument == Supported::D17) {
    const std::string chopperGapUnit = m_localWorkspace->getInstrument()->getStringParameter("chopper_gap_unit")[0];
    const double scale = (chopperGapUnit == "cm") ? 0.01 : (chopperGapUnit == "mm") ? 0.001 : 1.;
    double pairCentre;
    double pairSeparation;
    if (run.hasProperty("VirtualChopper.dist_chop_samp")) {
      // This is valid up to cycle 191 included
      pairCentre = doubleFromRun("VirtualChopper.dist_chop_samp"); // in [m]
      // It is in meter, just restate its unit
      m_localWorkspace->mutableRun().addProperty("VirtualChopper.dist_chop_samp", pairCentre, "meter", true);
      pairSeparation = doubleFromRun("Distance.ChopperGap") * scale; // [cm] to [m]
      // Here it's the first chopper to sample, so we need to subtract half of
      // the gap
      pairCentre -= 0.5 * pairSeparation;
    } else if (run.hasProperty("VirtualChopper.MidChopper_Sample_distance")) {
      // Valid from cycle 192 onwards, here it's directly the mid-chopper to
      // sample, but in mm
      pairCentre = mmToMeter(doubleFromRun("VirtualChopper.MidChopper_Sample_distance")); // [mm] to [m]
      pairSeparation = doubleFromRun("Distance.ChopperGap") * scale;                      // in [m]
      m_localWorkspace->mutableRun().addProperty("VirtualChopper.MidChopper_Sample_distance", pairCentre, "meter",
                                                 true);
    } else if (run.hasProperty("Distance.Chopper1_Sample")) {
      // Valid from cycle 212 onwards
      pairCentre = mmToMeter(doubleFromRun("Distance.MidChopper_Sample")); // [mm] to [m]
      pairSeparation = doubleFromRun("Distance.ChopperGap") * scale;       // in [m]
      m_localWorkspace->mutableRun().addProperty("VirtualChopper.MidChopper_Sample_distance", pairCentre, "meter",
                                                 true);
    } else {
      throw std::runtime_error("Unable to extract chopper to sample distance");
    }
    // in any case we overwrite the chopper gap now in meters, so that the
    // reduction code works universally
    m_localWorkspace->mutableRun().addProperty("Distance.ChopperGap", pairSeparation, "meter", true);
    return pairCentre;
  } else {
    if (run.hasProperty("ChopperSetting.chopperpair_sample_distance")) { // until cycle 231
      const double chopperDist = mmToMeter(doubleFromRun("ChopperSetting.chopperpair_sample_distance"));
      std::string entryName = "correct_chopper_sample_distance";
      bool correctChopperSampleDistance = m_localWorkspace->getInstrument()->getBoolParameter(entryName)[0];
      auto offset = 0.0;
      if (correctChopperSampleDistance) {
        const double deflectionAngle = doubleFromRun(m_sampleAngleName);
        offset = m_sampleZOffset / std::cos(degToRad(deflectionAngle));
      }
      return chopperDist + offset;
    } else if (run.hasProperty("Distance.MidChopper_Sample")) { // since cycle 231
      return mmToMeter(doubleFromRun("Distance.MidChopper_Sample"));
    } else {
      throw std::runtime_error("Unable to extract chopper to sample distance");
    }
  }
}

} // namespace Mantid::DataHandling
