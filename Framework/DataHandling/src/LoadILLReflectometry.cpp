#include "MantidDataHandling/LoadILLReflectometry.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::LinearGenerator;
using Mantid::DataObjects::create;
using Mantid::DataObjects::Workspace2D;

namespace {
constexpr double inRad(const double x) {
  return x * M_PI / 180;
}

constexpr double inDeg(const double x) {
  return x * 180 / M_PI;
}

constexpr double inMeter(const double x) {
  return x * 1e-3;
}

/** Computes the coherence and incoherence equation.
 *  @param angle the angle to compute the equation for.
 *  @param peakPos1 first peak position.
 *  @param peakPos2 second peak position.
 *  @param pixelCentre centre of the detector, in pixels.
 *  @param pixelWidth physical pixel width.
 *  @param detDist1 sample to detector distance corresponding to first peak.
 *  @param detDist2 sample to detector distance corresponding to second peak.
 *  @param sign a sign option depending on Figaro's reflection down option.
 *  @return the Bragg angle in degrees.
 */
double coherenceIncoherenceEq(const double angle, const double peakPos1, const double peakPos2, const double pixelCentre, const double pixelWidth, const double detDist1, const double detDist2, const double sign) {
  const double angle1 = std::atan2((peakPos1 - pixelCentre) * pixelWidth, detDist1);
  const double angle2 = std::atan2((peakPos2 - pixelCentre) * pixelWidth, detDist2);
  return inDeg(inRad(angle) - sign * 0.5 * (angle1 + sign * angle2));
}
}

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLReflectometry)

/**
 * Return the confidence with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLReflectometry::confidence(
    Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the ILL
  if ((descriptor.pathExists("/entry0/wavelength") || // ILL D17
       descriptor.pathExists("/entry0/theta"))        // ILL Figaro
      &&
      descriptor.pathExists("/entry0/experiment_identifier") &&
      descriptor.pathExists("/entry0/mode") &&
      (descriptor.pathExists("/entry0/instrument/VirtualChopper") || // ILL D17
       descriptor.pathExists("/entry0/instrument/Theta")) // ILL Figaro
      )
    return 80;
  else
    return 0;
}

/// Initialize the algorithm's properties.
void LoadILLReflectometry::init() {
  declareProperty(Kernel::make_unique<FileProperty>("Filename", std::string(),
                                                    FileProperty::Load, ".nxs",
                                                    Direction::Input),
                  "Name of the Nexus file to load");

  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", std::string(), Direction::Output),
                  "Name of the output workspace");

  const std::vector<std::string> angle{"sample angle", "detector angle",
                                       "user defined"};
  declareProperty("InputAngle", "sample angle",
                  boost::make_shared<StringListValidator>(angle),
                  "Optional angle for calculating the Bragg angle.\n");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0.0);
  declareProperty("BraggAngle", EMPTY_DBL(), positiveDouble,
                  "User defined Bragg angle");
  setPropertySettings("BraggAngle",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "InputAngle", IS_EQUAL_TO, "user defined"));

  const std::vector<std::string> availableUnits{"Wavelength", "TimeOfFlight"};
  declareProperty("XUnit", "Wavelength",
                  boost::make_shared<StringListValidator>(availableUnits),
                  "X unit of the OutputWorkspace");

  const std::vector<std::string> scattering{"coherent", "incoherent"};
  declareProperty("ScatteringType", "incoherent",
                  boost::make_shared<StringListValidator>(scattering),
                  "Scattering type used to calculate the Bragg angle");

  // user defined InputAngle and D17 instrument: ScatteringAngle can be disabled

  declareProperty(Kernel::make_unique<FileProperty>("DirectBeam", std::string(),
                                                    FileProperty::OptionalLoad,
                                                    ".nxs", Direction::Input),
                  "Name of the direct beam Nexus file to load");
  setPropertySettings("DirectBeam",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "InputAngle", IS_EQUAL_TO, "detector angle"));
}

/**
 * Validate inputs
 * @returns a string map containing the error messages
 */
std::map<std::string, std::string> LoadILLReflectometry::validateInputs() {
  std::map<std::string, std::string> result;
  // check input file
  const std::string fileName{getPropertyValue("Filename")};
  if (!fileName.empty() &&
      (m_supportedInstruments.find(fileName) != m_supportedInstruments.end()))
    result["Filename"] = "Instrument not supported.";
  // check user defined angle
  const double angleUserDefined{getProperty("BraggAngle")};
  const std::string angleOption{getPropertyValue("InputAngle")};
  if ((angleOption == "user defined") && (angleUserDefined == EMPTY_DBL()))
    result["BraggAngle"] =
        "User defined BraggAngle option requires an input value";
  // check direct beam file
  const std::string directBeam{getPropertyValue("DirectBeam")};
  if (!directBeam.empty() &&
      (m_supportedInstruments.find(directBeam) != m_supportedInstruments.end()))
    result["DirectBeam"] = "Instrument not supported.";
  // compatibility check for reflected and direct beam located in loadBeam
  // further input validation is needed for general LoadDialog and Python
  if ((angleOption != "user defined") && (angleUserDefined != EMPTY_DBL()))
    result["BraggAngle"] = "No input value required";
  if (directBeam.empty() && (angleOption == "detector angle"))
    result["InputAngle"] = "DirectBeam input required";
  return result;
}

/// Execute the algorithm.
void LoadILLReflectometry::exec() {
  // open the root node
  NeXus::NXRoot root(getPropertyValue("Filename"));
  NXEntry firstEntry{root.openFirstEntry()};
  // load Monitor details: n. monitors x monitor contents
  std::vector<std::vector<int>> monitorsData{loadMonitors(firstEntry)};
  // set instrument specific names of Nexus file entries
  initNames(firstEntry);
  // load Data details (number of tubes, channels, etc)
  loadDataDetails(firstEntry);
  // initialise workspace
  initWorkspace(monitorsData);
  // load the instrument from the IDF if it exists
  loadInstrument();
  // get properties
  loadNexusEntriesIntoProperties();
  // load data into the workspace
  loadData(firstEntry, monitorsData, getXValues());
  root.close();
  firstEntry.close();
  // position the source
  placeSource();
  // position the detector
  placeDetector();
  convertTofToWavelength();
  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
} // exec

/// Run the Child Algorithm LoadInstrument.
void LoadILLReflectometry::loadInstrument() {
  // execute the Child Algorithm. Catch and log any error, but don't stop.
  g_log.debug("Loading instrument definition...");
  try {
    IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(true));
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->executeAsChildAlg();
  } catch (std::runtime_error &e) {
    g_log.information() << "Unable to succesfully run LoadInstrument child algorithm: " << e.what() << '\n';
  }
}

/**
  * Init names of member variables based on instrument specific NeXus file
  * entries
  *
  * @params entry :: the NeXus file entry
  */
void LoadILLReflectometry::initNames(NeXus::NXEntry &entry) {
  std::string instrumentNamePath = m_loader.findInstrumentNexusPath(entry);
  m_instrumentName = entry.getString(instrumentNamePath.append("/name"));
  if (m_instrumentName.empty())
    throw std::runtime_error(
        "Cannot set the instrument name from the Nexus file!");
  // In NeXus files names are: D17 and figaro. The instrument
  // definition is independent and names start with a capital letter. This
  // loader follows its convention.
  boost::to_lower(m_instrumentName);
  m_instrumentName[0] = char((std::toupper(m_instrumentName[0])));
  g_log.debug() << "Instrument name: " << m_instrumentName << '\n';
  if (m_instrumentName == "D17") {
    m_detectorDistance = "det";
    m_detectorAngleName = "dan.value";
    m_sampleAngleName = "san.value";
    m_offsetFrom = "VirtualChopper";
    m_offsetName = "open_offset";
    m_pixelCentre = 135.75; // or 135.75 or 132.5
    m_chopper1Name = "Chopper1";
    m_chopper2Name = "Chopper2";
    // m_wavelength = getFloat("wavelength");
  } else if (m_instrumentName == "Figaro") {
    m_detectorDistance = "DTR";
    m_detectorAngleName = "VirtualAxis.DAN_actual_angle";
    m_sampleAngleName = "CollAngle.actual_coll_angle";
    m_offsetFrom = "CollAngle";
    m_offsetName = "openOffset";
    m_pixelCentre = double(m_numberOfHistograms) / 2.0;
    // Figaro: find out which of the four choppers are used
    NXFloat firstChopper =
        entry.openNXFloat("instrument/ChopperSetting/firstChopper");
    firstChopper.load();
    NXFloat secondChopper =
        entry.openNXFloat("instrument/ChopperSetting/secondChopper");
    secondChopper.load();
    m_chopper1Name = "CH" + std::to_string(int(firstChopper[0]));
    m_chopper2Name = "CH" + std::to_string(int(secondChopper[0]));
  }
  // get acquisition mode
  NXInt acqMode = entry.openNXInt("acquisition_mode");
  acqMode.load();
  m_acqMode = acqMode[0];
  m_acqMode ? g_log.debug("TOF mode") : g_log.debug("Monochromatic Mode");
}

/// Call child algorithm ConvertUnits for conversion from TOF to wavelength
void LoadILLReflectometry::convertTofToWavelength() {
  if (m_acqMode && (getPropertyValue("XUnit") == "Wavelength")) {
    auto convertToWavelength =
        createChildAlgorithm("ConvertUnits", -1, -1, true);
    convertToWavelength->initialize();
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                                           m_localWorkspace);
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                                           m_localWorkspace);
    convertToWavelength->setPropertyValue("Target", "Wavelength");
    convertToWavelength->executeAsChildAlg();
  }
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param monitorsData :: Monitors data already loaded
 */
void LoadILLReflectometry::initWorkspace(
    const std::vector<std::vector<int>> &monitorsData) {

  g_log.debug() << "Number of monitors: " << monitorsData.size() << '\n';
  for (size_t i = 0; i < monitorsData.size(); ++i) {
    if (monitorsData[i].size() != m_numberOfChannels)
      g_log.debug() << "Data size of monitor ID " << i << " is " << monitorsData[i].size() << '\n';
  }
  // create the workspace
  try {
    m_localWorkspace = WorkspaceFactory::Instance().create(
        "Workspace2D", m_numberOfHistograms + monitorsData.size(),
        m_numberOfChannels + 1, m_numberOfChannels);
  } catch (std::out_of_range &) {
    throw std::runtime_error(
        "Workspace2D cannot be created, check number of histograms (" +
            std::to_string(m_numberOfHistograms) +
            "), monitors (" +
            std::to_string(monitorsData.size()) +
            "), and channels (" +
            std::to_string(m_numberOfChannels) + '\n');
  }
  if (m_acqMode)
    m_localWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnitLabel("Counts");
  m_localWorkspace->mutableRun().addProperty("Facility", std::string("ILL"));
  if (m_wavelength > 0.0) {
    double ei = m_loader.calculateEnergy(m_wavelength);
    m_localWorkspace->mutableRun().addProperty<double>("Ei", ei, true);
  }
}

/**
 * Load Data details (number of tubes, channels, etc)
 *
 * @param entry First entry of nexus file
 */
void LoadILLReflectometry::loadDataDetails(NeXus::NXEntry &entry) {
  // PSD data D17 256 x 1 x 1000
  // PSD data Figaro 1 x 256 x 1000

  if (m_acqMode) {
    NXFloat timeOfFlight = entry.openNXFloat("instrument/PSD/time_of_flight");
    timeOfFlight.load();
    m_channelWidth = static_cast<double>(timeOfFlight[0]);
    m_numberOfChannels = size_t(timeOfFlight[1]);
    m_tofDelay = timeOfFlight[2];
    if (m_instrumentName == "Figaro") {
      NXFloat eDelay = entry.openNXFloat("instrument/Theta/edelay_delay");
      eDelay.load();
      m_tofDelay += static_cast<double>(eDelay[0]);
    }
  } else { // monochromatic mode
    m_numberOfChannels = 1;
  }

  NXInt nChannels = entry.openNXInt("instrument/PSD/detsize");
  nChannels.load();
  m_numberOfHistograms = nChannels[0];

  std::string widthName{};
  if (m_instrumentName == "D17")
    widthName = "mppx";
  else if (m_instrumentName == "Figaro")
    widthName = "mppy";

  NXFloat pixelWidth =
      entry.openNXFloat("instrument/PSD/" + widthName);
  pixelWidth.load();
  m_pixelWidth = inMeter(static_cast<double>(pixelWidth[0]));

  g_log.debug() << "Please note that ILL reflectometry instruments have "
              "several tubes, after integration one "
              "tube remains in the Nexus file.\n Number of tubes (banks): 1\n";
  g_log.debug() << "Number of pixels per tube (number of detectors and number "
           "of histograms): "
           << m_numberOfHistograms << '\n';
  g_log.debug() << "Number of time channels: " << m_numberOfChannels << '\n';
  g_log.debug() << "Channel width: " << m_channelWidth << " 1e-6 sec\n";
  g_log.debug() << "TOF delay: " << m_tofDelay << '\n';
  g_log.debug() << "Pixel width: " << m_pixelWidth << '\n';
}

double LoadILLReflectometry::doubleFromRun(const std::string &entryName) {
  return m_localWorkspace->run().getPropertyValueAsType<double>(entryName);
}

/**
 * Load single monitor
 *
 * @param entry :: The Nexus entry
 * @param monitor_id :: A std::string containing the Nexus path to the monitor
 *data
 * @return monitor :: A std::vector containing monitor values
 */
std::vector<int>
LoadILLReflectometry::loadSingleMonitor(NeXus::NXEntry &entry,
                                        const std::string &monitor_data) {
  NXData dataGroup = entry.openNXData(monitor_data);
  NXInt data = dataGroup.openIntData();
  // load counts
  data.load();
  std::vector<int> monitor(data(), data() + data.size());
  return monitor;
}

/**
 * Load monitor data
 *
 * @param entry :: The Nexus entry
 * @return :: A std::vector of vectors of monitors containing monitor values
 */
std::vector<std::vector<int>>
LoadILLReflectometry::loadMonitors(NeXus::NXEntry &entry) {
  g_log.debug("Read monitor data...");
  // vector of monitors with one entry
  std::vector<std::vector<int>> monitors{
      loadSingleMonitor(entry, "monitor1/data"),
      loadSingleMonitor(entry, "monitor2/data")};
  return monitors;
}

/**
 * Determine x values (unit time-of-flight)
 *
 * @return :: vector holding the x values
 */
std::vector<double> LoadILLReflectometry::getXValues() {
  std::vector<double> xVals;                  // no initialisation
  xVals.reserve(int(m_numberOfChannels) + 1); // reserve memory
  try {
    if (m_acqMode) {
      std::string chopper{"Chopper"};
      double chop1Speed{0.0}, chop2Speed{0.0}, chop2Phase{0.0};
      if (m_instrumentName == "D17") {
        chop1Speed = doubleFromRun("VirtualChopper.chopper1_speed_average");
        chop2Speed = doubleFromRun("VirtualChopper.chopper2_speed_average");
        chop2Phase = doubleFromRun("VirtualChopper.chopper2_phase_average");
      }
      // use phase of first chopper
      double chop1Phase = doubleFromRun(m_chopper1Name + ".phase");
      if (m_instrumentName == "Figaro" && chop1Phase > 360.0) {
        // Chopper 1 phase on Figaro is set to an arbitrary value (999.9)
        chop1Phase = 0.0;
      }
      double POFF = doubleFromRun(m_offsetFrom + ".poff");
      double openOffset = doubleFromRun(m_offsetFrom + "." + m_offsetName);
      if (m_instrumentName == "D17" && chop1Speed != 0.0 && chop2Speed != 0.0 && chop2Phase != 0.0) {
        // virtual chopper entries are valid
        chopper = "Virtual chopper";
      } else {
        // use chopper values
        chop1Speed = doubleFromRun(m_chopper1Name + ".rotation_speed");
        chop2Speed = doubleFromRun(m_chopper2Name +  ".rotation_speed");
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
      const double t_TOF2 = -1.e+6 * 60.0 *
                 (POFF - 45.0 + chop2Phase - chop1Phase + openOffset) /
                 (2.0 * 360 * chop1Speed);
      g_log.debug() << "t_TOF2: " << t_TOF2 << '\n';
      // compute tof values
      for (int channelIndex = 0; channelIndex <= int(m_numberOfChannels);
           ++channelIndex) {
        double t_TOF1 = (channelIndex + 0.5) * m_channelWidth + m_tofDelay;
        xVals.push_back(t_TOF1 + t_TOF2);
      }
    } else {
      g_log.debug("Time channel index for axis description \n");
      for (size_t t = 0; t <= m_numberOfChannels; ++t)
        xVals.push_back(double(t));
    }
  } catch (std::runtime_error &e) {
    g_log.information() << "Unable to access NeXus file entry: " << e.what() << '\n';
  }
  return xVals;
}

/**
 * Load data from nexus file
 *
 * @param entry :: The Nexus file entry
 * @param monitorsData :: Monitors data already loaded
 * @param xVals :: X values
 */
void LoadILLReflectometry::loadData(NeXus::NXEntry &entry,
                                    const std::vector<std::vector<int>> &monitorsData,
                                    const std::vector<double> &xVals) {
  g_log.debug("Loading data...");
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();
  size_t nb_monitors = monitorsData.size();
  Progress progress(this, 0, 1, m_numberOfHistograms + nb_monitors);

  // write monitors
  if (!xVals.empty()) {
    HistogramData::BinEdges binEdges(xVals);
    for (size_t im = 0; im < nb_monitors; im++) {
      const int *monitor_p = monitorsData[im].data();
      const Counts counts(monitor_p, monitor_p + m_numberOfChannels);
      m_localWorkspace->setHistogram(im, binEdges, std::move(counts));
      progress.report();
    }
    // write data
    for (size_t j = 0; j < m_numberOfHistograms; ++j) {
      const int *data_p = &data(0, static_cast<int>(j), 0);
      const Counts counts(data_p, data_p + m_numberOfChannels);
      m_localWorkspace->setHistogram((j + nb_monitors), binEdges,
                                     std::move(counts));
      progress.report();
    }
  } else
    g_log.debug("Vector of x values is empty");
} // LoadILLIndirect::loadData

/**
 * Use the LoadHelper utility to load most of the nexus entries into workspace
 * sample log properties
 */
void LoadILLReflectometry::loadNexusEntriesIntoProperties() {
  g_log.debug("Building properties...");
  // Open NeXus file
  const std::string filename{getPropertyValue("Filename")};
  NXhandle nxfileID;
  NXstatus stat = NXopen(filename.c_str(), NXACC_READ, &nxfileID);
  if (stat == NX_ERROR)
    throw Kernel::Exception::FileError("Unable to open File:", filename);
  m_loader.addNexusFieldsToWsRun(nxfileID, m_localWorkspace->mutableRun());
  stat = NXclose(&nxfileID);
}

/** Load direct or reflected beam:
  * - load detector counts
  * - get angle value for computing the Bragg angle, only for direct beam
  * @params beamWS :: Workspace holding detector counts
  * @params beam :: Name of the beam file
  * @params angleDirectBeam :: Name of the detector angle of the direct beam
  */
void LoadILLReflectometry::loadBeam(MatrixWorkspace_sptr &beamWS,
                                    const std::string &beam,
                                    std::string angleDirectBeam) {
  if (!beam.empty()) {
    // init beam workspace, we do not need its monitor counts
    beamWS = WorkspaceFactory::Instance().create(
        "Workspace2D", m_numberOfHistograms, m_numberOfChannels + 1,
        m_numberOfChannels);
    // open the root node
    NeXus::NXRoot dataRoot(getPropertyValue(beam));
    NXEntry entry{dataRoot.openFirstEntry()};
    // load counts
    NXData dataGroup = entry.openNXData("data");
    NXInt data = dataGroup.openIntData();
    data.load();
    // check whether beam workspace is compatible
    if (beam == "DirectBeam") {
      if (data.dim0() * data.dim1() * data.dim2() !=
          int(m_numberOfChannels * m_numberOfHistograms))
        g_log.error(beam + " has incompatible size with Filename beam\n");
      // get sample detector distance
      if (m_instrumentName == "D17")
        m_detectorDistanceDirectBeam = inMeter(entry.getFloat(
            "instrument/" + m_detectorDistance + "/value"));
      else if (m_instrumentName == "Figaro")
        m_detectorDistanceDirectBeam =
            inMeter(entry.getFloat("instrument/" +  m_detectorDistance + "/value")) +
            inMeter(entry.getFloat("instrument/" + m_detectorDistance + "/offset_value"));
      g_log.debug() << "Sample-detector distance " << beam << " : " << m_detectorDistanceDirectBeam << '\n';
      // set Bragg angle of the direct beam for later use
      if (!angleDirectBeam.empty()) {
        std::replace(angleDirectBeam.begin(), angleDirectBeam.end(), '.', '/');
        m_angleDirectBeam =
            entry.getFloat("instrument/" + angleDirectBeam);
        g_log.debug() << "Bragg angle of the direct beam: " << m_angleDirectBeam << '\n';
      }
    }
    /* uncommented since validation needed. This cannot be found in cosmos
    // set offset angle
    if (!(incidentAngle == "user defined")) {
      double sampleAngle = getDouble(m_sampleAngleName); // read directly from
    Nexus
    file
      double detectorAngle = getDouble(m_detectorAngleName); // read directly
    from Nexus file
      debugLog("sample angle ", sampleAngle);
      debugLog("detector angle ", detectorAngle);
      m_offsetAngle = detectorAngle / 2. * sampleAngle;
      debugLogWithUnitDegrees("Offset angle of the direct beam (will be added to
    "
                              "the scattering angle) ",
                              m_offsetAngle);
    }
    */
    dataRoot.close();
    // set x values
    std::vector<double> xVals;
    xVals.reserve(int(m_numberOfChannels) + 1);
    for (size_t t = 0; t < m_numberOfChannels + 1; ++t)
      xVals.push_back(double(t));
    // write data
    if (!xVals.empty()) {
      HistogramData::BinEdges binEdges(xVals);
      for (size_t j = 0; j < m_numberOfHistograms; ++j) {
        int *data_p = &data(0, static_cast<int>(j), 0);
        const Counts counts(data_p, data_p + m_numberOfChannels);
        beamWS->setHistogram(j, binEdges, std::move(counts));
      }
    }
  } else
    throw std::runtime_error("Name of the beam is missing");
}

/**
  * Gaussian fit to determine peak position AND set the Bragg angle of
  *the direct beam if requested for later use
  *
  * @param beam :: Name of the beam. This is the ReflectedBeam by default and
  *the DirectBeam if explicitely mentioned
  * @param angleDirectBeam :: Name of detector angle of the direct beam
  * @return centre :: detector position of the peak: Gaussian fit and position
  *of the maximum (serves as start value for the optimization)
  */
std::vector<double>
LoadILLReflectometry::fitReflectometryPeak(const std::string &beam,
                                           const std::string &angleDirectBeam) {
  std::vector<double> centre{0.0, 0.0};
  if ((beam == "DirectBeam") || (beam == "Filename")) {
    MatrixWorkspace_sptr beamWS;
    loadBeam(beamWS, beam, angleDirectBeam);
    Points x(m_numberOfHistograms, LinearGenerator(0, 1));
    auto singleSpectrum = create<Workspace2D>(1, Histogram(x));
    MatrixWorkspace_sptr spectrum{std::move(singleSpectrum)};
    for (size_t i = 0; i < (m_numberOfHistograms); ++i) {
      auto Y = beamWS->y(i);
      spectrum->mutableY(0)[i] = std::accumulate(Y.begin(), Y.end(), 0);
    }
    // check sum of detector counts
    if ((beam == "Filename") &&
        doubleFromRun("PSD.detsum") !=
            std::accumulate(spectrum->y(0).begin(), spectrum->y(0).end(), 0))
      g_log.error("Error after integrating and transposing beam\n");
    // determine initial height: maximum value
    auto maxValueIt =
        std::max_element(spectrum->y(0).begin(), spectrum->y(0).end());
    double height = *maxValueIt;
    // determine initial centre: index of the maximum value
    size_t maxIndex = std::distance(spectrum->y(0).begin(), maxValueIt);
    centre[1] = static_cast<double>(maxIndex);
    g_log.debug() << "Peak maximum position of " << beam << ": " << centre[1] << '\n';
    // determine sigma
    auto lessThanHalfMax = [height](const double x) { return x < 0.5 * height; };
    using IterType = HistogramData::HistogramY::const_iterator;
    std::reverse_iterator<IterType> revMaxValueIt{maxValueIt};
    auto revMinFwhmIt = std::find_if(revMaxValueIt, spectrum->y(0).crend(), lessThanHalfMax);
    auto maxFwhmIt = std::find_if(maxValueIt, spectrum->y(0).cend(), lessThanHalfMax);
    std::reverse_iterator<IterType> revMaxFwhmIt{maxFwhmIt};
    const double fwhm = static_cast<double>(std::distance(revMaxFwhmIt, revMinFwhmIt) + 1);
    g_log.debug() << "Initial fwhm (fixed window at half maximum) " << beam << ": " << fwhm << '\n';
    // generate Gaussian
    auto func = API::FunctionFactory::Instance().createFunction("Gaussian");
    auto initialGaussian =
        boost::dynamic_pointer_cast<API::IPeakFunction>(func);
    initialGaussian->setHeight(height);
    initialGaussian->setCentre(centre[1]);
    initialGaussian->setFwhm(fwhm);
    // call Fit child algorithm
    API::IAlgorithm_sptr fitGaussian =
        createChildAlgorithm("Fit", -1, -1, true);
    fitGaussian->initialize();
    fitGaussian->setProperty(
        "Function",
        boost::dynamic_pointer_cast<API::IFunction>(initialGaussian));
    fitGaussian->setProperty("InputWorkspace", spectrum);
    bool success = fitGaussian->execute();
    if (!success)
      g_log.warning("Fit not successful, take initial values\n");
    else {
      // get fitted values back
      centre[0] = initialGaussian->centre();
      double sigma = initialGaussian->fwhm();
      g_log.debug() << "Sigma: " << sigma << '\n';
    }
    g_log.debug() << "Estimated peak position of " << beam << ": " << centre[0] << '\n';
  } else
    throw std::runtime_error(
        "The input " + beam + " does not exist");
  return centre;
}

/// Compute Bragg angle
double LoadILLReflectometry::computeBraggAngle() {
  // compute bragg angle called angleBragg in the following
  const std::string inputAngle = getPropertyValue("InputAngle");
  std::string incidentAngle{std::string()};
  if (inputAngle == "sample angle" || inputAngle == "detector angle") {
    inputAngle == "sample angle" ? incidentAngle = m_sampleAngleName
                                 : incidentAngle = m_detectorAngleName;
  } else
    incidentAngle = "user defined";
  double angle = getProperty("BraggAngle");
  // no user input for BraggAngle means we take sample or detector angle value
  if (angle == EMPTY_DBL()) {
    // modify error message "Unknown property search object"
    if (m_localWorkspace->run().hasProperty(incidentAngle)) {
      angle = doubleFromRun(incidentAngle);
      g_log.debug() << "Use angle " << incidentAngle << ": " << angle << " degrees.\n";
    } else
      throw std::runtime_error(
          incidentAngle + " is not defined in Nexus file");
  }
  // user angle and sample angle behave equivalently for D17
  const std::string scatteringType = getProperty("ScatteringType");
  double angleBragg{angle};
  // the reflected beam
  std::vector<double> peakPosRB = fitReflectometryPeak("Filename");
  // Figaro theta sign informs about reflection down (-1.0) or up (1.0)
  double down{1.0}; // default value for D17
  if (m_instrumentName == "Figaro") {
    down = doubleFromRun("theta");
    down > 0. ? down = 1. : down = -1.;
    if (inputAngle == "detectorAngle")
      down = down;
  }
  double sign{-down};
  if (((inputAngle == ("sample angle")) || (m_instrumentName == "Figaro")) &&
      (scatteringType == "coherent")) {
    angleBragg = coherenceIncoherenceEq(angle, peakPosRB[1], peakPosRB[0], m_pixelCentre, m_pixelWidth, m_detectorDistanceValue, m_detectorDistanceValue, sign);
  } else if (inputAngle == "detector angle") {
    // DirectBeam is abvailable and we can read from its Nexus file
    std::vector<double> peakPosDB =
        fitReflectometryPeak("DirectBeam", incidentAngle);
    const double angleCentre = down * (angle - m_angleDirectBeam) / 2.;
    g_log.debug() << "Centre angle " << angleCentre << " degrees.\n";
    if (scatteringType == "incoherent") {
      angleBragg = coherenceIncoherenceEq(angleCentre, peakPosDB[0], peakPosRB[0], m_pixelCentre, m_pixelWidth, m_detectorDistanceDirectBeam, m_detectorDistanceValue, sign);
    } else if (scatteringType == "coherent") {
      angleBragg = coherenceIncoherenceEq(angleCentre, peakPosDB[0], peakPosRB[1] + 0.5, m_pixelCentre, m_pixelWidth, m_detectorDistanceDirectBeam, m_detectorDistanceValue, sign);
    }
  }
  g_log.debug() << "Bragg angle " << angleBragg << " degrees.\n";
  return angleBragg;
}

/// Update detector position according to data file
void LoadILLReflectometry::placeDetector() {
  g_log.debug("Move the detector bank \n");
  double dist = doubleFromRun(m_detectorDistance + ".value");
  m_detectorDistanceValue = inMeter(dist);
  if (m_instrumentName == "Figaro")
    dist += doubleFromRun(m_detectorDistance + ".offset_value");
  g_log.debug() << "Sample-detector distance: " << m_detectorDistanceValue << "m.\n";
  const double rho = computeBraggAngle() + m_offsetAngle / 2.;
  const double theta = (2. * rho);
  // incident angle for using the algorithm ConvertToReflectometryQ
  m_localWorkspace->mutableRun().addProperty("stheta", inRad(rho));
  const std::string componentName = "detector";
  V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
  V3D newpos(m_detectorDistanceValue * sin(inRad(theta)), pos.Y(),
             m_detectorDistanceValue * cos(inRad(theta)));
  m_loader.moveComponent(m_localWorkspace, componentName, newpos);
  // apply a local rotation to stay perpendicular to the beam
  const V3D axis(0.0, 1.0, 0.0);
  const Quat rotation(2. * rho, axis);
  m_loader.rotateComponent(m_localWorkspace, componentName, rotation);
}

/// Update source position.
void LoadILLReflectometry::placeSource() {
  if (m_instrumentName != "Figaro") {
    return;
  }
  const double dist = inMeter(doubleFromRun("ChopperSetting.chopperpair_sample_distance"));
  g_log.debug() << "Source-sample distance " << dist << "m.\n";
  const std::string source = "chopper1";
  const V3D newPos{0.0, 0.0, -dist};
  m_loader.moveComponent(m_localWorkspace, source, newPos);
}
} // namespace DataHandling
} // namespace Mantid
