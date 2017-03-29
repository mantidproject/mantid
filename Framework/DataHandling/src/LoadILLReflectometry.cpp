#include "MantidDataHandling/LoadILLReflectometry.h"

#include "MantidAPI/Axis.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
//#include "MantidDataObjects/WorkspaceCreation.h"

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
  // add input validation for det option: only figaro
  // compatibility check for reflected and direct beam
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
    g_log.information()
        << "Unable to successfully run LoadInstrument Child Algorithm : "
        << e.what() << '\n';
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLReflectometry::exec() {
  // retrieve filename
  const std::string filenameData{getPropertyValue("Filename")};
  // open the root node
  NeXus::NXRoot dataRoot(filenameData);
  NXEntry firstEntry{dataRoot.openFirstEntry()};
  // load Monitor details: n. monitors x monitor contents
  std::vector<std::vector<int>> monitorsData{loadMonitors(firstEntry)};
  // set instrument name
  std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath.append("/name"));
  // load Data details (number of tubes, channels, etc)
  loadDataDetails(firstEntry);
  // initialise workspace
  initWorkspace(monitorsData);
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
    g_log.debug() << "No TOF values for axis description (no "
                     "conversion to wavelength possible). \n";
    xVals.reserve(m_numberOfChannels + 1);
    for (size_t t = 0; t <= m_numberOfChannels; ++t) {
      xVals.push_back(double(t));
    }
  }
  // load data into the workspace
  loadData(firstEntry, monitorsData, xVals);
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
    g_log.error(message);
    throw std::runtime_error(message);
  }
  m_instrumentName =
      m_loader.getStringFromNexusPath(firstEntry, instrumentNamePath);
  boost::to_lower(m_instrumentName);
  g_log.debug() << "Instrument name : " + m_instrumentName << '\n';
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param monitorsData :: Monitors data already loaded
 */
void LoadILLReflectometry::initWorkspace(
    std::vector<std::vector<int>> monitorsData) {

  g_log.debug() << "Number of monitors: " << monitorsData.size() << '\n';
  for (size_t i = 0; i < monitorsData.size(); ++i) {
    if (monitorsData[i].size() != m_numberOfChannels)
      g_log.debug() << "Data size of monitor" << i << ": "
                    << monitorsData[i].size() << '\n';
  }
  // create the workspace
  try {
    m_localWorkspace = WorkspaceFactory::Instance().create(
        "Workspace2D", m_numberOfHistograms + monitorsData.size(),
        m_numberOfChannels + 1, m_numberOfChannels);
  } catch (std::out_of_range &) {
    std::string info = "Workspace2D cannot be created, check number of "
                       "histograms (";
    std::string message = info.append(std::to_string(m_numberOfHistograms))
                              .append("), monitors (")
                              .append(std::to_string(monitorsData.size()))
                              .append("), and channels (")
                              .append(std::to_string(m_numberOfChannels))
                              .append(")\n");
    throw std::runtime_error(message);
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

  g_log.debug() << "Please note that ILL reflectometry instruments have "
                   "several tubes, after integration one "
                   "tube remains in the Nexus file.\n";
  g_log.debug() << "Number of tubes (banks): 1\n";
  g_log.debug() << "Number of pixels per tube (number of detectors and number "
                   "of histograms): " << m_numberOfHistograms << '\n';
  g_log.debug() << "Number of time channels: " << m_numberOfChannels << '\n';
  g_log.debug() << "Channel width: " << m_channelWidth << " 10e-6 sec\n";
  g_log.debug() << "TOF delay: " << m_tofDelay << "\n";
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
  // load detector counts
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
    std::string ch1{std::string()}, ch2{std::string()},
        offsetFrom{std::string()}, offsetName{std::string()};
    std::string chopper;
    PropertyWithValue<double> *chop1_speed{NULL}, *chop2_speed{NULL},
        *chop2_phase{NULL};
    if (m_instrumentName == "figaro") {
      // figaro: find out which of the four choppers are used
      auto firstChopper = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("ChopperSetting.firstChopper"));
      auto secondChopper = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("ChopperSetting.secondChopper"));
      std::string chopper1{"CH"}, chopper2{"CH"};
      ch1 = chopper1.append(std::to_string(int(*firstChopper)));
      ch2 = chopper2.append(std::to_string(int(*secondChopper)));
      offsetFrom = "CollAngle";
      offsetName = "openOffset";
      chopper = "Chopper";
    } else if (m_instrumentName == "d17") {
      ch1 = "Chopper1";
      ch2 = "Chopper2";
      offsetFrom = "VirtualChopper";
      offsetName = "open_offset";
      chop1_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(
              "VirtualChopper.chopper1_speed_average"));
      chop2_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(
              "VirtualChopper.chopper2_speed_average"));
      chop2_phase = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(
              "VirtualChopper.chopper2_phase_average"));
    }
    // use phase of first chopper
    std::string ch1phase{ch1};
    auto chop1_phase = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(ch1phase.append(".phase")));
    std::string poff{offsetFrom}, openOffset{offsetFrom};
    auto POFF = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(poff.append(".poff")));
    auto open_offset = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(
            openOffset.append(".").append(offsetName)));
    if (chop1_speed && chop2_speed && chop2_phase && *chop1_speed != 0.0 &&
        *chop1_speed != 0.0 && *chop2_phase != 0.0) { // only for d17
      // virtual chopper entries are valid
      chopper = "Virtual chopper";
    } else {
      // use chopper values
      std::string ch1speed{ch1}, ch2speed{ch2}, ch2phase{ch2};
      chop1_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(
              ch1speed.append(".rotation_speed")));
      chop2_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(
              ch2speed.append(".rotation_speed")));
      chop2_phase = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(ch2phase.append(".phase")));
      chopper = "Chopper";
    }
    // logging
    g_log.debug() << "Poff: " << *POFF << '\n';
    g_log.debug() << "Open offset: " << *open_offset << '\n';
    g_log.debug() << "Chopper 1 phase : " << *chop1_phase << '\n';
    g_log.debug() << chopper << " 1 speed : " << *chop1_speed << '\n';
    g_log.debug() << chopper << " 2 phase : " << *chop2_phase << '\n';
    g_log.debug() << chopper << " 2 speed : " << *chop2_speed << '\n';

    /*
    temp 1 = parref.tofd / cos(atan(((peakref – parref.x_min) *
    parref.pixeldensity - (c_params.pixels_x / 2.)-0.5) * c_params.pixelwidth,
    parref.tofd))
    //lambda = 1e10 * (c_params.planckperkg * ((findgen(tsize) + 0.5) *
    parref.channelwidth + parref.delay) / temp1)
    */
    double t_TOF2{0.0};
    if (chop1_speed && chop1_phase && chop2_phase && open_offset && POFF &&
        *chop1_speed != 0.0) {
      t_TOF2 = -1.e+6 * 60.0 *
               (*POFF - 45.0 + *chop2_phase - *chop1_phase + *open_offset) /
               (2.0 * 360 * *chop1_speed);
    }
    g_log.debug() << "t_TOF2 : " << t_TOF2 << '\n';
    if (!t_TOF2)
      g_log.warning() << "TOF values may be incorrect, check chopper values\n";
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
    g_log.information() << "Unable to access Nexus file entry : " << e.what()
                        << '\n';
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
                                    std::vector<std::vector<int>> monitorsData,
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

  HistogramData::BinEdges binEdges(xVals);
  // write monitors
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
  if (stat == NX_ERROR) {
    g_log.debug() << "convertNexusToProperties: Error loading " << filename;
    throw Kernel::Exception::FileError("Unable to open File:", filename);
  }
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
  const std::string theBeam{getPropertyValue(beam)};
  // init beam workspace, we do not need its monitor counts
  beamWS = WorkspaceFactory::Instance().create(
      "Workspace2D", m_numberOfHistograms, m_numberOfChannels + 1,
      m_numberOfChannels);
  // set x values
  std::vector<double> xVals;
  xVals.reserve(m_numberOfChannels + 1);
  for (size_t t = 0; t <= m_numberOfChannels; ++t) {
    xVals.push_back(double(t));
  }
  // open the root node
  NeXus::NXRoot dataRoot(theBeam);
  NXEntry entry{dataRoot.openFirstEntry()};
  // load counts
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  data.load();
  // check whether beam workspace is compatible
  if (beam == "DirectBeam") {
    if (data.dim0() * data.dim1() * data.dim2() !=
        int(m_numberOfChannels * m_numberOfHistograms))
      g_log.error() << beam
                    << " has incompatible size with beam read from Filename\n";
    // set Bragg angle of the direct beam for later use
    if (!angleDirectBeam.empty()) {
      std::replace(angleDirectBeam.begin(), angleDirectBeam.end(), '.', '/');
      m_BraggAngleDirectBeam =
          entry.getFloat(std::string("instrument/").append(angleDirectBeam));
      g_log.debug() << "Bragg angle of the direct beam: "
                    << m_BraggAngleDirectBeam << " degrees\n"; //?
    }
  }
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
}

/**
  * Gaussian fit to determine peak position AND set the Bragg angle of
  *the direct beam if requested for later use
  *
  * @param beam :: Name of the beam. This is the ReflectedBeam by default and
  *the DirectBeam if explicitely mentioned
  * @param angleDirectBeam :: Name of the angle for calculating the Bragg angle.
  *This is particularly useful in case of the detector angle.
  * @return centre :: detector position of the peak
  */
double
LoadILLReflectometry::fitReflectometryPeak(const std::string beam,
                                           const std::string angleDirectBeam) {
  MatrixWorkspace_sptr beamWS;
  if (beam == "DirectBeam") {
    loadBeam(beamWS, beam, angleDirectBeam);
  } else {
    // m_localWorkspace cannot be used, even when not considering monitors
    loadBeam(beamWS, "Filename", angleDirectBeam);
  }
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
       std::accumulate(oneSpectrum->y(0).begin(), oneSpectrum->y(0).end(), 0)))
    g_log.error("Error after integrating and transposing beam\n");
  // determine initial height: maximum value
  auto maxValueIt = std::max_element(spectrum.begin(), spectrum.end());
  double height = *maxValueIt;
  // determine initial centre: index of the maximum value
  size_t maxIndex = std::distance(spectrum.begin(), maxValueIt);
  double centre = static_cast<double>(maxIndex);
  // determine sigma
  auto minFwhmIt =
      std::find_if(maxValueIt, spectrum.begin(),
                   [height](double value) { return value < 0.5 * height; });
  auto maxFwhmIt =
      std::find_if(maxValueIt, spectrum.end(),
                   [height](double value) { return value < 0.5 * height; });
  double sigma =
      0.5 * static_cast<double>(std::distance(minFwhmIt, maxFwhmIt) + 1);
  // generate Gaussian
  auto func = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto initialGaussian = boost::dynamic_pointer_cast<API::IPeakFunction>(func);
  initialGaussian->setHeight(height);
  initialGaussian->setCentre(centre);
  initialGaussian->setFwhm(sigma);
  g_log.debug() << "Initial peak position: " << centre << "\n";
  // call Fit child algorithm
  API::IAlgorithm_sptr fitGaussian = createChildAlgorithm("Fit", -1, -1, true);
  fitGaussian->initialize();
  fitGaussian->setProperty(
      "Function", boost::dynamic_pointer_cast<API::IFunction>(initialGaussian));
  fitGaussian->setProperty("InputWorkspace", oneSpectrum);
  bool success = fitGaussian->execute();
  if (!success) {
    g_log.warning() << "Fit not successful, take initial values \n";
  } else {
    // get fitted values back
    centre = initialGaussian->centre();
    sigma = initialGaussian->fwhm();
    g_log.debug() << "Sigma: " << sigma << "\n";
  }
  g_log.debug() << "Estimated peak position of " << beam << ": " << centre
                << "\n";
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
    std::string detectorAngleName{std::string()};
    if (m_instrumentName == "d17")
      detectorAngleName = "dan.value";
    else // figaro
      detectorAngleName = "VirtualAxis.dan_actual_angle";
    thetaIn == "sample angle" ? thetaAngle = "san.value"
                              : thetaAngle = detectorAngleName;
  } else
    thetaAngle = "user defined";
  double theta = getProperty("BraggAngle");
  // no user input for theta means we take sample or detector angle value
  if (theta == EMPTY_DBL()) {
    // error message without this check would be: Unknown property search
    // object ... and thus not be meaningful
    if (m_localWorkspace->run().hasProperty(thetaAngle)) {
      theta =
          m_localWorkspace->run().getPropertyValueAsType<double>(thetaAngle);
    } else {
      throw std::runtime_error("BraggAngleIs (sample or detector option) "
                               "is not defined in Nexus file");
    }
  }
  // user angle and sample angle behave equivalently for d17
  const std::string scatteringType = getProperty("ScatteringType");
  if (thetaIn == "detector angle") {
    double fittedPeakPosDB = fitReflectometryPeak("DirectBeam", thetaAngle);
    // the reflected beam
    double fittedPeakPosRB = fitReflectometryPeak("Filename");
    double angleCentre = ((theta - m_BraggAngleDirectBeam) / 2.) * M_PI / 180.;
    double d17pcen = 135.79;
    g_log.debug() << "Center angle " << angleCentre << "\n";
    // sdetd = par1[25]*1.0e-3
    // figaro c_params.pcen = (float(par1[100]-par1[99])/2)-0.5
    // sdetd = par1[18]*1.0e-3
    if (scatteringType == "incoherent") {
      /*
      theta = angleCentre +
      0.5*atan((fittedPeakPosDB-pcen)*pixelwidth/pardir.sdetd)  -
      0.5*atan((fittedPeakPosRB-pcen)*pixelwidth/parref.sdetd)
      */
    } else if (scatteringType == "coherent") {
      /*
      theta = angleCentre +
      0.5*atan((fittedPeakPosDB-pcen)*pixelwidth/pardir.sdetd)  -
      0.5*atan((((peakref)+0.5)-pcen)*pixelwidth/parref.sdetd)
      we have to use the middle of the coherent center pixel
      */
    }
  } else if (scatteringType == "coherent") {
    /*
    peakref : centre of reflected peak in x direction
    theta = theta * M_PI / 180.) -
    0.5*atan((peakref-pcen)*c_params.pixelwidth/parref.sdetd)  +
    0.5*atan((fittedPeakPosRB-pcen)*c_params.pixelwidth/parref.sdetd)
    */
  }
  g_log.debug() << "Using " << thetaIn << " to calculate the Bragg angle "
                << theta << " degrees\n";
  return theta;
  /*
  Cosmos
  conversion to radians *pi/180
  conversion to grad    *180/pi
  parref: parameters from reflected beam
  bragg angle (rad), parref.san in grad

  detector pixels projected onto a given pixel, so we need
  to select middle of specular pixel (peakref) instead of value from fit
  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  ; calculate angle offset for flat detector surface
  function cosmos_anal_calcangleoffset, peakpos, xoff, density, distance
  return  atan(((peakpos - xoff) * density - (c_params.pixels_x / 2.)-0.5) *
  c_params.pixelwidth, distance)

  ; calculate corrected time of flight distance for flat detector surface
  cosmos_anal_correctdistance(peakpos, xoff, density, distance)
  return distance / cos(cosmos_anal_calcangleoffset(peakpos, xoff, density,
  distance))

  calculate lambda (wavelength in angstroms) determined for reflected beam
  only

  temp1 = cosmos_anal_correctdistance(peakref, parref.x_min,
  parref.pixeldensity, parref.tofd)
  temp2 = abs(temp1 - cosmos_anal_correctdistance(peakdir, pardir.x_min,
  pardir.pixeldensity, pardir.tofd))
  ' Difference in corrected TOF distance between direct and reflect beams is '
  temp2
  if (temp2 / temp1) gt 0.01 then ' Run no. ' runno ': Different TOF distances
  from direct and reflect runs.'
  lambda = 1e10 * (c_params.planckperkg * ((findgen(tsize) + 0.5) *
  parref.channelwidth + parref.delay) / temp1)
  */
}

/**
 * Utility to place detector in space, according to data file
 */
void LoadILLReflectometry::placeDetector() {
  g_log.debug() << "Move the detector bank \n";
  double theta =
      computeBraggAngle(); // can be in exec, theta can be a private variable
  double twotheta_rad = (2. * theta) * M_PI / 180.;
  // incident theta angle for easier calling the algorithm
  // ConvertToReflectometryQ
  m_localWorkspace->mutableRun().addProperty("stheta",
                                             double(twotheta_rad / 2.));
  std::string detectorAngleName{std::string()}, detectorDistance{std::string()};
  if (m_instrumentName == "d17") { // can be returned by computeBraggAngle
    detectorDistance = "det";
    detectorAngleName = "dan.value";
  } else { // figaro
    detectorDistance = "DTR";
    detectorAngleName = "VirtualAxis.dan_actual_angle";
  }
  double detectorAngle =
      m_localWorkspace->run().getPropertyValueAsType<double>(detectorAngleName);
  auto dist = dynamic_cast<PropertyWithValue<double> *>(
      m_localWorkspace->run().getProperty(detectorDistance.append(".value")));
  double distance = *dist / 1000.0; // convert to meter
  g_log.debug() << "Sample-detector distance in millimeter " << *dist << '\n';
  const std::string componentName = "bank";
  V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
  V3D newpos(distance * sin(twotheta_rad), pos.Y(),
             distance * cos(twotheta_rad));
  m_loader.moveComponent(m_localWorkspace, componentName, newpos);
  // offset angle
  double sampleAngle =
      m_localWorkspace->run().getPropertyValueAsType<double>("san.value");
  g_log.debug() << "sample angle " << sampleAngle << "\n";
  g_log.debug() << "detector angle " << detectorAngle << "\n";
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
