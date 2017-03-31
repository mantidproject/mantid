#include "MantidDataHandling/LoadILLReflectometry.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
//#include "MantidDataObjects/WorkspaceCreation.h"

// formula
#define atanFor(a)                                                             \
  atan((a - m_pixelCentre) * m_pixelWidth / m_detectorDistanceDirectBeam)
#define coherenceEq1(a, b, c) a + 0.5 * (atanFor(b) - atanFor(c))
#define coherenceEq2(a, b, c) a + 0.5 * (atanFor(b) + atanFor(c))
#define iterator(a, b, c)                                                      \
  std::find_if(a, b, [c](double value) { return value < 0.5 * c; })
// logging
#define debugLog(myString, myValue)                                            \
  g_log.debug(                                                                 \
      std::string(myString).append(std::to_string(myValue)).append("\n"))
#define debugLog2(myString, aString, myValue)                                  \
  g_log.debug(std::string(myString)                                            \
                  .append(aString)                                             \
                  .append(": ")                                                \
                  .append(std::to_string(myValue))                             \
                  .append("\n"))
#define infoLog(myString)                                                      \
  g_log.information(std::string(myString).append(e.what()).append("\n"))
// get value from sample logs of the (output) workspace
#define getValue(myString)                                                     \
  dynamic_cast<PropertyWithValue<double> *>(                                   \
      m_localWorkspace->run().getProperty(myString))

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLReflectometry)

//----------------------------------------------------------------------------------------------
/**
 * Return the confidence with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLReflectometry::confidence(
    Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the ILL
  if ((descriptor.pathExists("/entry0/wavelength") || // ILL d17
       descriptor.pathExists("/entry0/theta"))        // ILL figaro
      &&
      descriptor.pathExists("/entry0/experiment_identifier") &&
      descriptor.pathExists("/entry0/mode") &&
      (descriptor.pathExists("/entry0/instrument/VirtualChopper") || // ILL d17
       descriptor.pathExists("/entry0/instrument/Theta")) // ILL figaro
      )
    return 80;
  else
    return 0;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLReflectometry::init() {
  declareProperty(Kernel::make_unique<FileProperty>("Filename", std::string(),
                                                    FileProperty::Load, ".nxs",
                                                    Direction::Input),
                  "File path of the data file to load");

  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", std::string(), Direction::Output),
                  "The name to use for the output workspace");

  const std::vector<std::string> theta{"sample angle", "detector angle",
                                       "user defined"};
  declareProperty("BraggAngleIs", "sample angle",
                  boost::make_shared<StringListValidator>(theta),
                  "Optional angle for calculating the scattering angle.\n");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(1.0);
  declareProperty(
      "BraggAngle", EMPTY_DBL(), positiveDouble,
      "User defined angle in degrees for computing the scattering angle");
  setPropertySettings("BraggAngle",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "BraggAngleIs", IS_EQUAL_TO, "user defined"));

  const std::vector<std::string> availableUnits{"Wavelength", "TimeOfFlight"};
  declareProperty("XUnit", "Wavelength",
                  boost::make_shared<StringListValidator>(availableUnits),
                  "X unit of the OutputWorkspace");

  const std::vector<std::string> scattering{"coherent", "incoherent"};
  declareProperty("ScatteringType", "incoherent",
                  boost::make_shared<StringListValidator>(scattering),
                  "Scattering type used to calculate the scattering angle");

  declareProperty(Kernel::make_unique<FileProperty>("DirectBeam", std::string(),
                                                    FileProperty::OptionalLoad,
                                                    ".nxs", Direction::Input),
                  "File path of the direct beam file to load");
  setPropertySettings("DirectBeam",
                      Kernel::make_unique<EnabledWhenProperty>(
                          "BraggAngleIs", IS_EQUAL_TO, "detector angle"));
}

//----------------------------------------------------------------------------------------------
/** Validate inputs
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
  const double thetaUserDefined{getProperty("BraggAngle")};
  const std::string angleOption{getPropertyValue("BraggAngleIs")};
  if ((angleOption == "user defined") && (thetaUserDefined == EMPTY_DBL()))
    result["BraggAngle"] =
        "User defined BraggAngle option requires an input value";
  // check direct beam file
  const std::string directBeam{getPropertyValue("DirectBeam")};
  if (!directBeam.empty() &&
      (m_supportedInstruments.find(directBeam) != m_supportedInstruments.end()))
    result["DirectBeam"] = "Instrument not supported.";
  // compatibility check for reflected and direct beam in loadBeam
  return result;
}

//----------------------------------------------------------------------------------------------
/**
   * Run the Child Algorithm LoadInstrument.
   */
void LoadILLReflectometry::runLoadInstrument() {
  // execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(true));
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->executeAsChildAlg();
  } catch (std::runtime_error &e) {
    infoLog("Unable to successfully run LoadInstrument Child Algorithm : ");
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLReflectometry::exec() {
  // open the root node
  NeXus::NXRoot dataRoot(getPropertyValue("Filename"));
  NXEntry firstEntry{dataRoot.openFirstEntry()};
  // load Monitor details: n. monitors x monitor contents
  std::vector<std::vector<int>> monitorsData{loadMonitors(firstEntry)};
  // set instrument name
  std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, std::string(instrumentPath).append("/name"));
  // load Data details (number of tubes, channels, etc)
  loadDataDetails(firstEntry);
  // initialise workspace
  initWorkspace(monitorsData);
  // set instrument specific names of Nexus file entries
  initNames(firstEntry);
  // get properties
  loadNexusEntriesIntoProperties(firstEntry);
  // get acquisition mode
  m_acqMode =
      m_localWorkspace->run().getPropertyAsIntegerValue("acquisition_mode");
  m_acqMode ? g_log.debug("TOF mode") : g_log.debug("Monochromatic Mode");
  // load the instrument from the IDF if it exists
  g_log.debug("Loading instrument definition...");
  runLoadInstrument(); // ExperimentInfo
  // get TOF values -> move out of exec
  std::vector<double> xVals;
  if (m_channelWidth) {
    getXValues(xVals);
  } else {
    g_log.debug("No TOF values for axis description (no "
                "conversion to wavelength possible). \n");
    xVals.reserve(m_numberOfChannels + 1);
    for (size_t t = 0; t <= m_numberOfChannels; ++t) {
      xVals.push_back(double(t));
    }
  }
  // load data into the workspace
  loadData(firstEntry, monitorsData, xVals);
  dataRoot.close();
  // position the detector
  placeDetector();
  const std::string unit = getPropertyValue("XUnit");
  if (m_channelWidth && m_acqMode && (unit == "Wavelength")) {
    convertToWavelength();
  }
  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
  * Init names of instrument specific NeXus file entries
  */
void LoadILLReflectometry::initNames(NeXus::NXEntry &entry) {
  if (m_instrumentName == "d17") {
    m_detectorDistance = "det";
    m_detectorAngleName = "dan.value";
    m_offsetFrom = "VirtualChopper";
    m_offsetName = "open_offset";
    m_pixelCentre = 135.75;
    m_chopper1Name = "Chopper1";
    m_chopper2Name = "Chopper2";
  } else if (m_instrumentName == "figaro") {
    m_detectorDistance = "DTR";
    m_detectorAngleName = "VirtualAxis.dan_actual_angle";
    m_offsetFrom = "CollAngle";
    m_offsetName = "openOffset";
    m_pixelCentre = double(m_numberOfHistograms) / 2.0;
    // figaro: find out which of the four choppers are used
    NXFloat firstChopper =
        entry.openNXFloat("instrument/ChopperSetting/firstChopper");
    firstChopper.load();
    NXFloat secondChopper =
        entry.openNXFloat("instrument/ChopperSetting/secondChopper");
    secondChopper.load();
    m_chopper1Name =
        std::string("CH").append(std::to_string(int(firstChopper[0])));
    m_chopper2Name =
        std::string("CH").append(std::to_string(int(secondChopper[0])));
  }
}

/**
  * Call child algorithm ConvertUnits for conversion from TOF to wavelength
  */
void LoadILLReflectometry::convertToWavelength() {
  auto convertToWavelength = createChildAlgorithm("ConvertUnits", -1, -1, true);
  convertToWavelength->initialize();
  convertToWavelength->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                                         m_localWorkspace);
  convertToWavelength->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                                         m_localWorkspace);
  convertToWavelength->setPropertyValue("Target", "Wavelength");
  convertToWavelength->executeAsChildAlg();
}

/**
 * Set member variable with the instrument name
 */
void LoadILLReflectometry::setInstrumentName(
    const NeXus::NXEntry &firstEntry, const std::string &instrumentNamePath) {

  if (instrumentNamePath == std::string()) {
    std::string message("Cannot set the instrument name from the Nexus file!");
    throw std::runtime_error(message);
  }
  m_instrumentName =
      m_loader.getStringFromNexusPath(firstEntry, instrumentNamePath);
  boost::to_lower(m_instrumentName);
  g_log.debug(
      std::string("Instrument name : ").append(m_instrumentName).append("\n"));
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param monitorsData :: Monitors data already loaded
 */
void LoadILLReflectometry::initWorkspace(
    std::vector<std::vector<int>> monitorsData) {

  debugLog("Number of monitors: ", monitorsData.size());
  for (size_t i = 0; i < monitorsData.size(); ++i) {
    if (monitorsData[i].size() != m_numberOfChannels)
      debugLog2("Data size of monitor", std::to_string(i),
                monitorsData[i].size());
  }
  // create the workspace
  try {
    m_localWorkspace = WorkspaceFactory::Instance().create(
        "Workspace2D", m_numberOfHistograms + monitorsData.size(),
        m_numberOfChannels + 1, m_numberOfChannels);
  } catch (std::out_of_range &) {
    throw std::runtime_error(
        std::string("Workspace2D cannot be created, check number of "
                    "histograms (")
            .append(std::to_string(m_numberOfHistograms))
            .append("), monitors (")
            .append(std::to_string(monitorsData.size()))
            .append("), and channels (")
            .append(std::to_string(m_numberOfChannels))
            .append(")\n"));
  }
  if (m_acqMode)
    m_localWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Load Data details (number of tubes, channels, etc)
 *
 * @param entry First entry of nexus file
 */
void LoadILLReflectometry::loadDataDetails(NeXus::NXEntry &entry) {
  // PSD data D17 256 x 1 x 1000
  // PSD data figaro 1 x 256 x 1000

  NXFloat timeOfFlight = entry.openNXFloat("instrument/PSD/time_of_flight");
  timeOfFlight.load();
  m_channelWidth = static_cast<double>(timeOfFlight[0]);
  m_numberOfChannels = size_t(timeOfFlight[1]);
  m_tofDelay = timeOfFlight[2];

  NXInt nChannels = entry.openNXInt("instrument/PSD/detsize");
  nChannels.load();
  m_numberOfHistograms = nChannels[0];

  NXFloat pixelWidth = entry.openNXFloat("instrument/PSD/mppy");
  pixelWidth.load();
  m_pixelWidth = static_cast<double>(pixelWidth[0]) * 1.0e-3;

  g_log.debug("Please note that ILL reflectometry instruments have "
              "several tubes, after integration one "
              "tube remains in the Nexus file.\n Number of tubes (banks): 1\n");
  debugLog("Number of pixels per tube (number of detectors and number "
           "of histograms): ",
           m_numberOfHistograms);
  debugLog("Number of time channels: ", m_numberOfChannels);
  g_log.debug() << "Channel width: " << m_channelWidth << " 10e-6 sec\n";
  debugLog("TOF delay: ", m_tofDelay);
  g_log.debug() << "Pixel width " << m_pixelWidth << " m\n";
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
                                        std::string monitor_data) {
  NXData dataGroup = entry.openNXData(monitor_data);
  NXInt data = dataGroup.openIntData();
  // load counts
  data.load();
  std::vector<int> monitor(data(), data() + data.size());
  return monitor;
}

/**
 * Load monitors data found in nexus file
 *
 * @param entry :: The Nexus entry
 * @return :: A std::vector of vectors containing values from all monitors
 */
std::vector<std::vector<int>>
LoadILLReflectometry::loadMonitors(NeXus::NXEntry &entry) {
  // read in the data
  g_log.debug("Fetching monitor data...");
  // vector of monitors with one entry
  std::vector<std::vector<int>> monitors{
      loadSingleMonitor(entry, "monitor1/data"),
      loadSingleMonitor(entry, "monitor2/data")};
  return monitors;
}

/**
 * Determine x values
 *
 * @param xVals :: vector holding the x values
 */
void LoadILLReflectometry::getXValues(std::vector<double> &xVals) {
  try {
    std::string chopper{"Chopper"};
    PropertyWithValue<double> *chop1_speed{NULL}, *chop2_speed{NULL},
        *chop2_phase{NULL};
    if (m_instrumentName == "d17") {
      chop1_speed = getValue("VirtualChopper.chopper1_speed_average");
      chop2_speed = getValue("VirtualChopper.chopper2_speed_average");
      chop2_phase = getValue("VirtualChopper.chopper2_phase_average");
    }
    // use phase of first chopper
    auto chop1_phase = getValue(std::string(m_chopper1Name).append(".phase"));
    auto POFF = getValue(std::string(m_offsetFrom).append(".poff"));
    auto open_offset =
        getValue(std::string(m_offsetFrom).append(".").append(m_offsetName));
    if (chop1_speed && chop2_speed && chop2_phase && *chop1_speed != 0.0 &&
        *chop1_speed != 0.0 && *chop2_phase != 0.0) { // only for d17
      // virtual chopper entries are valid
      chopper = "Virtual chopper";
    } else {
      // use chopper values
      chop1_speed = getValue(std::string(m_chopper1Name).append(".rotation_speed"));
      chop2_speed = getValue(std::string(m_chopper2Name).append(".rotation_speed"));
      chop2_phase = getValue(std::string(m_chopper2Name).append(".phase"));
    }
    // logging
    debugLog("Poff: ", *POFF);
    debugLog("Open offset: ", *open_offset);
    debugLog("Chopper 1 phase : ", *chop1_phase);
    debugLog(chopper + " 1 speed : ", *chop1_speed);
    debugLog(chopper + " 2 phase : ", *chop2_phase);
    debugLog(chopper + " 2 speed : ", *chop2_speed);

    double t_TOF2{0.0};
    if (chop1_speed && chop1_phase && chop2_phase && open_offset && POFF &&
        *chop1_speed != 0.0) {
      t_TOF2 = -1.e+6 * 60.0 *
               (*POFF - 45.0 + *chop2_phase - *chop1_phase + *open_offset) /
               (2.0 * 360 * *chop1_speed);
    }
    debugLog("t_TOF2 : ", t_TOF2);
    if (!t_TOF2)
      g_log.warning("TOF values may be incorrect, check chopper values\n");
    // compute tof values
    xVals.reserve(m_numberOfChannels + 1);
    for (size_t timechannelnumber = 0; timechannelnumber <= m_numberOfChannels;
         ++timechannelnumber) {
      double t_TOF1 =
          (static_cast<int>(timechannelnumber) + 0.5) * m_channelWidth +
          m_tofDelay;
      xVals.push_back(t_TOF1 + t_TOF2);
    }
  } catch (std::runtime_error &e) {
    infoLog("Unable to access Nexus file entry : ");
  }
}

/**
 * Load data from nexus file
 *
 * @param entry :: The Nexus file entry
 * @param monitorsData :: Monitors data already loaded
 * @param xVals :: X values
 */
void LoadILLReflectometry::loadData(NeXus::NXEntry &entry,
                                    std::vector<std::vector<int>> &monitorsData,
                                    std::vector<double> &xVals) {
  g_log.debug("Loading data...");
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();
  size_t nb_monitors = monitorsData.size();
  Progress progress(this, 0, 1, m_numberOfHistograms + nb_monitors);

  if (m_instrumentName == "d17") {
    m_wavelength = entry.getFloat("wavelength");
    double ei = m_loader.calculateEnergy(m_wavelength);
    m_localWorkspace->mutableRun().addProperty<double>("Ei", ei,
                                                       true); // overwrite
  }
  // write monitors
  HistogramData::BinEdges binEdges(xVals);
  for (size_t im = 0; im < nb_monitors; im++) {
    int *monitor_p = monitorsData[im].data();
    const HistogramData::Counts histoCounts(monitor_p,
                                            monitor_p + m_numberOfChannels);
    m_localWorkspace->setHistogram(im, binEdges, std::move(histoCounts));
    progress.report();
  }
  // write data
  size_t spec = 0;
  for (size_t j = 0; j < m_numberOfHistograms; ++j) {
    int *data_p = &data(0, static_cast<int>(j), 0);
    const HistogramData::Counts histoCounts(data_p,
                                            data_p + m_numberOfChannels);
    m_localWorkspace->setHistogram((spec + nb_monitors), binEdges,
                                   std::move(histoCounts));
    ++spec;
    progress.report();
  }
} // LoadILLIndirect::loadData

/**
 * Use the LoadHelper utility to load most of the nexus entries into workspace
 * log properties
 */
void LoadILLReflectometry::loadNexusEntriesIntoProperties(
    NeXus::NXEntry &entry) {

  /*
  g_log.debug("Building properties...");
  std::string parameterStr;
  ::NeXus::File *file;
  file->openGroup(entry, "NXentry");
  try {
    // load logs, sample, and instrument
    m_localWorkspace->loadExperimentInfoNexus(getPropertyValue("Filename"),
                                              file, parameterStr);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    g_log.information("Error loading experiment info of nxs file");
  }
  */
  const std::string filename{getPropertyValue("Filename")};
  API::Run &runDetails = m_localWorkspace->mutableRun();
  // Open NeXus file
  NXhandle nxfileID;
  NXstatus stat = NXopen(filename.c_str(), NXACC_READ, &nxfileID);
  if (stat == NX_ERROR)
    throw Kernel::Exception::FileError("Unable to open File:", filename);
  m_loader.addNexusFieldsToWsRun(nxfileID, runDetails);
  runDetails.addProperty("Facility", std::string("ILL"));
  stat = NXclose(&nxfileID);
}

/**
  * Load direct or reflected beam:
  * - load detector counts
  * - get angle value for computing the Bragg angle, only for direct beam
  * @params beamWS :: workspace holding detector counts
  * @params beam :: name of the beam file
  * @params angleDirectBeam :: if the beam is a direct beam, the name of the
  * angle used for computing the Bragg angle
  */
void LoadILLReflectometry::loadBeam(MatrixWorkspace_sptr &beamWS,
                                    const std::string beam,
                                    std::string angleDirectBeam) {
  if (!beam.empty()) {
    // init beam workspace, we do not need its monitor counts
    beamWS = WorkspaceFactory::Instance().create(
        "Workspace2D", m_numberOfHistograms, m_numberOfChannels + 1,
        m_numberOfChannels);
    // set x values
    std::vector<double> xVals;
    xVals.reserve(m_numberOfChannels + 1);
    for (size_t t = 0; t < m_numberOfChannels + 1; ++t) {
      xVals.push_back(double(t));
    }
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
        g_log.error()
            << beam << " has incompatible size with beam read from Filename\n";
      // get sample detector distance
      std::replace(m_detectorDistance.begin(), m_detectorDistance.end(), '.',
                   '/');
      m_detectorDistanceDirectBeam =
          entry.getFloat(std::string("instrument/")
                             .append(m_detectorDistance)
                             .append("/value"));
      // set Bragg angle of the direct beam for later use
      if (!angleDirectBeam.empty()) {
        std::replace(angleDirectBeam.begin(), angleDirectBeam.end(), '.', '/');
        m_BraggAngleDirectBeam =
            entry.getFloat(std::string("instrument/").append(angleDirectBeam));
        g_log.debug() << "Bragg angle of the direct beam: "
                      << m_BraggAngleDirectBeam << " degrees\n"; //?
      }
    }
    dataRoot.close();
    // write data
    HistogramData::BinEdges binEdges(xVals);
    size_t spec = 0;
    for (size_t j = 0; j < m_numberOfHistograms; ++j) {
      int *data_p = &data(0, static_cast<int>(j), 0);
      const HistogramData::Counts histoCounts(data_p,
                                              data_p + m_numberOfChannels);
      beamWS->setHistogram(spec, binEdges, std::move(histoCounts));
      ++spec;
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
  * @param angleDirectBeam :: Name of the angle for calculating the Bragg angle.
  *This is particularly useful in case of the detector angle.
  * @return centre :: detector position of the peak: Gaussian fit and position
  *of the maximum
  */
std::vector<double>
LoadILLReflectometry::fitReflectometryPeak(const std::string beam,
                                           const std::string angleDirectBeam) {
  std::vector<double> centre{0.0, 0.0};
  if ((beam == "DirectBeam") || (beam == "Filename")) {
    MatrixWorkspace_sptr beamWS;
    loadBeam(beamWS, beam, angleDirectBeam);
    // create new MatrixWorkspace containing point data (one spectrum only)
    MatrixWorkspace_sptr oneSpectrum = WorkspaceFactory::Instance().create(
        "Workspace2D", 1, m_numberOfHistograms, m_numberOfHistograms);
    // Points x(m_numberOfHistograms, LinearGenerator(0, 1));
    // Counts y(m_numberOfHistograms, LinearGenerator(0, 1));
    // CountStandardDeviations e(m_numberOfHistograms, LinearGenerator(0, 1));
    // create<Workspace2D>(1, Histogram(x, y, e));
    for (size_t i = 0; i < (m_numberOfHistograms); ++i) {
      auto Y = beamWS->y(i);
      oneSpectrum->mutableY(0)[i] = std::accumulate(Y.begin(), Y.end(), 0);
    }
    auto spectrum = oneSpectrum->y(0);
    // check sum of detector counts
    if ((beam == "Filename") &&
        (m_localWorkspace->run().getPropertyValueAsType<double>("PSD.detsum") !=
         std::accumulate(oneSpectrum->y(0).begin(), oneSpectrum->y(0).end(),
                         0)))
      g_log.error("Error after integrating and transposing beam\n");
    // determine initial height: maximum value
    auto maxValueIt = std::max_element(spectrum.begin(), spectrum.end());
    double height = *maxValueIt;
    // determine initial centre: index of the maximum value
    size_t maxIndex = std::distance(spectrum.begin(), maxValueIt);
    centre[1] = static_cast<double>(maxIndex);
    // determine sigma
    auto minFwhmIt = iterator(maxValueIt, spectrum.begin(), height);
    auto maxFwhmIt = iterator(maxValueIt, spectrum.end(), height);
    double sigma =
        0.5 * static_cast<double>(std::distance(minFwhmIt, maxFwhmIt) + 1);
    // generate Gaussian
    auto func = API::FunctionFactory::Instance().createFunction("Gaussian");
    auto initialGaussian =
        boost::dynamic_pointer_cast<API::IPeakFunction>(func);
    initialGaussian->setHeight(height);
    initialGaussian->setCentre(centre[1]);
    initialGaussian->setFwhm(sigma);
    debugLog2("Position of the peak maximum value (initial peak position) of ",
              beam, centre[1]);
    // call Fit child algorithm
    API::IAlgorithm_sptr fitGaussian =
        createChildAlgorithm("Fit", -1, -1, true);
    fitGaussian->initialize();
    fitGaussian->setProperty(
        "Function",
        boost::dynamic_pointer_cast<API::IFunction>(initialGaussian));
    fitGaussian->setProperty("InputWorkspace", oneSpectrum);
    bool success = fitGaussian->execute();
    if (!success)
      g_log.warning("Fit not successful, take initial values\n");
    else {
      // get fitted values back
      centre[0] = initialGaussian->centre();
      sigma = initialGaussian->fwhm();
      debugLog("Sigma: ", sigma);
    }
    debugLog2("Estimated peak position of ", beam, centre[0]);
  } else
    throw std::runtime_error(
        std::string("The input ").append(beam).append(" does not exist"));
  return centre;
}

/**
  * Compute Bragg angle
  */
double LoadILLReflectometry::computeBraggAngle() {
  // compute bragg angle
  const std::string thetaIn = getPropertyValue("BraggAngleIs");
  std::string thetaAngle{std::string()};
  if (thetaIn == "sample angle" || thetaIn == "detector angle") {
    thetaIn == "sample angle" ? thetaAngle = "san.value"
                              : thetaAngle = m_detectorAngleName;
  } else
    thetaAngle = "user defined";
  double theta = getProperty("BraggAngle");
  // no user input for theta means we take sample or detector angle value
  if (theta == EMPTY_DBL()) {
    // error message without this check would be: Unknown property search
    // object ... and thus not be meaningful
    if (m_localWorkspace->run().hasProperty(thetaAngle))
      theta =
          m_localWorkspace->run().getPropertyValueAsType<double>(thetaAngle);
    else
      throw std::runtime_error("BraggAngleIs (sample or detector option) "
                               "is not defined in Nexus file");
  }
  // user angle and sample angle behave equivalently for d17
  const std::string scatteringType = getProperty("ScatteringType");
  // the reflected beam
  std::vector<double> peakPosRB = fitReflectometryPeak("Filename");
  if (thetaIn == "detector angle") {
    // DirectBeam is abvailable and we can read from its NeXus file
    std::vector<double> peakPosDB =
        fitReflectometryPeak("DirectBeam", thetaAngle);
    double angleCentre = ((theta - m_BraggAngleDirectBeam) / 2.) * M_PI / 180.;
    debugLog("Center angle ", angleCentre);
    if (scatteringType == "incoherent")
      theta = coherenceEq1(angleCentre, peakPosDB[0], peakPosRB[0]);
    else if (scatteringType == "coherent")
      theta = coherenceEq1(angleCentre, peakPosDB[0], peakPosRB[1]);
  } else if (scatteringType == "coherent")
    theta = coherenceEq2(theta * (M_PI / 180.), peakPosRB[1], peakPosRB[0]);
  g_log.debug() << "Using " << thetaIn << " to calculate the Bragg angle "
                << theta << " degrees\n";
  return theta;
}

/**
 * Utility to place detector in space, according to data file
 */
void LoadILLReflectometry::placeDetector() {
  g_log.debug("Move the detector bank \n");
  double detectorAngle = m_localWorkspace->run().getPropertyValueAsType<double>(
      m_detectorAngleName);
  auto dist = getValue(std::string(m_detectorDistance).append(".value"));
  m_detectorDistanceValue = *dist * 1.0e-3; // convert to meter
  debugLog("Sample-detector distance in meter ", m_detectorDistanceValue);
  double theta =
      computeBraggAngle(); // can be in exec, theta can be a private variable
  double twotheta_rad = (2. * theta) * M_PI / 180.;
  // incident theta angle for easier calling the algorithm
  // ConvertToReflectometryQ
  m_localWorkspace->mutableRun().addProperty("stheta",
                                             double(twotheta_rad / 2.));
  const std::string componentName = "bank";
  V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
  V3D newpos(m_detectorDistanceValue * sin(twotheta_rad), pos.Y(),
             m_detectorDistanceValue * cos(twotheta_rad));
  m_loader.moveComponent(m_localWorkspace, componentName, newpos);
  // offset angle
  double sampleAngle =
      m_localWorkspace->run().getPropertyValueAsType<double>("san.value");
  debugLog("sample angle ", sampleAngle);
  debugLog("detector angle ", detectorAngle);
  double offsetAngle = detectorAngle / 2. * sampleAngle;
  g_log.debug() << "Offset angle of the direct beam (will be added to the "
                   "scattering angle) " << offsetAngle << " degrees\n";
  // apply a local rotation to stay perpendicular to the beam
  const V3D axis(0.0, 1.0, 0.0);
  const Quat rotation(2. * theta + offsetAngle, axis);
  m_loader.rotateComponent(m_localWorkspace, componentName, rotation);
}
} // namespace DataHandling
} // namespace Mantid
