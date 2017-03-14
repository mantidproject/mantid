#include "MantidDataHandling/LoadILLReflectometry.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

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
  declareProperty(Kernel::make_unique<FileProperty>("Filename", "",
                                                    FileProperty::Load, ".nxs",
                                                    Direction::Input),
                  "File path of the data file to load");

  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  const std::vector<std::string> theta{"sample angle", "detector angle",
                                       "user defined"};
  declareProperty(
      "BraggAngleIs", "sample angle",
      boost::make_shared<StringListValidator>(theta),
      "Optional angle for calculating the scattering angle.\n"
      "Sample angle, detector angle or a user defined Bragg angle)");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(1.0);
  declareProperty("BraggAngle", EMPTY_DBL(), positiveDouble,
                  "If BraggAngleIs user defined, the must provide an angle"
                  " in degrees for computing the scattering angle");

  const std::vector<std::string> availableUnits{"Wavelength", "TimeOfFlight"};
  declareProperty("XUnit", "Wavelength",
                  boost::make_shared<StringListValidator>(availableUnits),
                  "X unit of the OutputWorkspace");

  const std::vector<std::string> scattering{"coherent", "incoherent"};
  declareProperty("ScatteringType", "incoherent",
                  boost::make_shared<StringListValidator>(scattering),
                  "Scattering type used to calculate the scattering angle");

  declareProperty(Kernel::make_unique<FileProperty>("DirectBeam", "",
                                                    FileProperty::OptionalLoad,
                                                    ".nxs", Direction::Input),
                  "File path of the direct beam file to load");
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
  if ((angleOption != "user defined") && (thetaUserDefined != EMPTY_DBL()))
    result["BraggAngle"] = "No input value required";
  // check direct beam file
  const std::string directBeam{getPropertyValue("DirectBeam")};
  if (!directBeam.empty() &&
      (m_supportedInstruments.find(directBeam) != m_supportedInstruments.end()))
    result["DirectBeam"] = "Instrument not supported.";
  if (directBeam.empty() && (angleOption == "detector angle")) {
    result["BraggAngleIs"] = "DirectBeam input required";
  }
  // add input validation for det option: only figaro
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
    loadInst->execute();
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
  setInstrumentName(firstEntry, instrumentPath);
  // load Data details (number of tubes, channels, etc)
  loadDataDetails(firstEntry);
  // initialise workspace
  initWorkspace(m_localWorkspace, monitorsData);
  // get properties
  loadNexusEntriesIntoProperties(filenameData);
  // get acquisition mode
  m_acqMode =
      m_localWorkspace->run().getPropertyAsIntegerValue("acquisition_mode");
  m_acqMode ? g_log.debug("TOF mode") : g_log.debug("Monochromatic Mode");
  // load the instrument from the IDF if it exists
  g_log.debug("Loading instrument definition...");
  runLoadInstrument();
  // position the detector
  placeDetector();
  // get TOF values
  std::vector<double> xVals;
  if (m_channelWidth) {
    m_localWorkspace->mutableRun().addProperty<double>(
        "channel_width", m_channelWidth, true); // overwrite
    getXValues(xVals);
  } else {
    g_log.debug() << "No TOF values for axis description (no "
                     "conversion to wavelength possible). \n";
    xVals.reserve(m_localWorkspace->x(0).size());
    for (size_t t = 0; t <= m_numberOfChannels; ++t) {
      double dt = double(t);
      xVals.push_back(dt);
    }
  }
  // load data into the workspace
  loadData(firstEntry, monitorsData, xVals);
  const std::string unit = getPropertyValue("XUnit");
  if (m_channelWidth > 0 && m_acqMode && (unit == "Wavelength")) {
    convertToWavelength();
  }
  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
  * Call child algorithm ConvertUnits for conversion from TOF to wavelength
  */
void LoadILLReflectometry::convertToWavelength() {
  auto convertToWavelength = createChildAlgorithm("ConvertUnits", true);
  convertToWavelength->initialize();
  convertToWavelength->setLogging(true);
  convertToWavelength->enableHistoryRecordingForChild(true);
  convertToWavelength->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                                         m_localWorkspace);
  convertToWavelength->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                                         m_localWorkspace);
  convertToWavelength->setPropertyValue("Target", "Wavelength");
  convertToWavelength->execute();
}

/**
 * Set member variable with the instrument name
 */
void LoadILLReflectometry::setInstrumentName(
    const NeXus::NXEntry &firstEntry, const std::string &instrumentNamePath) {

  if (instrumentNamePath == "") {
    std::string message("Cannot set the instrument name from the Nexus file!");
    g_log.error(message);
    throw std::runtime_error(message);
  }
  m_instrumentName =
      m_loader.getStringFromNexusPath(firstEntry, instrumentNamePath + "/name");
  boost::to_lower(m_instrumentName);
  g_log.debug() << "Instrument name : " + m_instrumentName << '\n';
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param workspace :: The Workspace2D to be created
 * @param monitorsData :: Monitors data already loaded
 */
void LoadILLReflectometry::initWorkspace(
    API::MatrixWorkspace_sptr &workspace,
    std::vector<std::vector<int>> monitorsData) {

  g_log.debug() << "Number of monitors: " << monitorsData.size() << '\n';
  for (size_t i = 0; i < monitorsData.size(); ++i) {
    if (monitorsData[i].size() != m_numberOfChannels)
      g_log.debug() << "Data size of monitor" << i << ": "
                    << monitorsData[i].size() << '\n';
  }
  // create the workspace
  try {
    workspace = WorkspaceFactory::Instance().create(
        "Workspace2D", m_numberOfHistograms + monitorsData.size(),
        m_numberOfChannels + 1, m_numberOfChannels);
  } catch (std::out_of_range &) {
    throw std::runtime_error(
        "Workspace2D cannot be created, check number of "
        "histograms (" +
        std::to_string(m_numberOfHistograms) + "), monitors (" +
        std::to_string(monitorsData.size()) + "), and channels (" +
        std::to_string(m_numberOfChannels) + ")\n");
  }
  if (m_acqMode)
    workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  workspace->setYUnitLabel("Counts");
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
  // load the counts from the file into memory
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
    std::string ch1{""}, ch2{""}, offsetFrom{""}, offsetName{""};
    std::string chopper;
    PropertyWithValue<double> *chop1_speed{NULL}, *chop2_speed{NULL},
        *chop2_phase{NULL};
    if (m_instrumentName == "figaro") {
      // figaro: find out which of the four choppers are used
      auto firstChopper = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("ChopperSetting.firstChopper"));
      auto secondChopper = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("ChopperSetting.secondChopper"));
      ch1 = "CH" + std::to_string(int(*firstChopper));
      ch2 = "CH" + std::to_string(int(*secondChopper));
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
    auto chop1_phase = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(ch1 + ".phase"));
    auto POFF = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(offsetFrom + ".poff"));
    auto open_offset = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(offsetFrom + "." + offsetName));

    if (chop1_speed && chop2_speed && chop2_phase && *chop1_speed != 0.0 &&
        *chop1_speed != 0.0 && *chop2_phase != 0.0) { // only for d17
      // virtual chopper entries are valid
      chopper = "Virtual chopper";
    } else {
      // use chopper values
      chop1_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(ch2 + ".rotation_speed"));
      chop2_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(ch2 + ".rotation_speed"));
      chop2_phase = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(ch2 + ".phase"));
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
    xVals.reserve(m_localWorkspace->x(0).size());
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
  } else if (m_instrumentName == "figaro") {
  }

} // LoadILLIndirect::loadData

/**
 * Use the LoadHelper utility to load most of the nexus entries into workspace
 * log properties
 */
void LoadILLReflectometry::loadNexusEntriesIntoProperties(
    std::string nexusfilename) {
  g_log.debug("Building properties...");
  API::Run &runDetails = m_localWorkspace->mutableRun();
  // Open NeXus file
  NXhandle nxfileID;
  NXstatus stat = NXopen(nexusfilename.c_str(), NXACC_READ, &nxfileID);
  if (stat == NX_ERROR) {
    g_log.debug() << "convertNexusToProperties: Error loading "
                  << nexusfilename;
    throw Kernel::Exception::FileError("Unable to open File:", nexusfilename);
  }
  m_loader.addNexusFieldsToWsRun(nxfileID, runDetails);
  runDetails.addProperty("Facility", std::string("ILL"));
  stat = NXclose(&nxfileID);
}

/**
  * Gaussian fit to determine peak position
  *
  * @param beam :: Name of the beam. This is the ReflectedBeam by default and
  *the DirectBeam if explicitely mentioned
  */
double LoadILLReflectometry::fitPeakPosition(std::string beam) {
  MatrixWorkspace_sptr beamWS;
  if (beam == "DirectBeam") {
    const std::string directBeam{getPropertyValue("DirectBeam")};
    // open the root node
    NeXus::NXRoot dataRoot(directBeam);
    NXEntry entry{dataRoot.openFirstEntry()};
    // load data into a Workspace2D
    NXData dataGroup = entry.openNXData("data");
    NXInt data = dataGroup.openIntData();
    // load the counts from the file into memory
    data.load();
    // load Monitor details: n. monitors x monitor contents
    std::vector<std::vector<int>> monitorsData{loadMonitors(entry)};
    // initialise workspace
    initWorkspace(beamWS, monitorsData);
  } else
    beamWS = m_localWorkspace;
  double peakPosition{0.0};
  // need transposed workspace for fitting routines
  // The algorithm FindReflectometryLines seems to be well suited, but does not
  // use Gaussian fitting

  return peakPosition;
}

/**
  * Compute Bragg angle
  */
double LoadILLReflectometry::computeBraggAngle() {
  // compute bragg angle
  const std::string thetaIn = getPropertyValue("BraggAngleIs");
  std::string thetaAngle{""};
  if (thetaIn == "sample angle" || thetaIn == "detector angle") {
    if (m_instrumentName == "d17") {
      thetaIn == "sample angle" ? thetaAngle = "san.value" : thetaAngle =
                                                                 "dan.value";
    } else {
      thetaIn == "sample angle" ? thetaAngle = "san.value"
                                : thetaAngle = "VirtualAxis.dan_actual_angle";
    }
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
    const std::string directBeam = getProperty("DirectBeam");
    Mantid::Kernel::NexusDescriptor descriptor(directBeam);
    if (descriptor.pathExists("/entry0/instrument/dan")) {
      auto thetaDirectBeam = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(thetaAngle)); // to check
      double angleCentre = ((theta - *thetaDirectBeam) / 2.) * M_PI / 180.;
      double fittedPeakPosDB = fitPeakPosition("DirectBeam");
      double fittedPeakPosRB = fitPeakPosition();
      double d17pcen = 135.79;
      // sdetd = par1[25]*1.0e-3
      // figaro c_params.pcen = (float(par1[100]-par1[99])/2)-0.5
      // sdetd = par1[18]*1.0e-3
    }
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
  double theta = computeBraggAngle();
  double twotheta_rad = (2. * theta) * M_PI / 180.;
  // incident theta angle for easier calling the algorithm
  // ConvertToReflectometryQ
  m_localWorkspace->mutableRun().addProperty("stheta",
                                             double(twotheta_rad / 2.));
  std::string detectorAngleName{""}, detectorDistance{""};
  if (m_instrumentName == "d17") {
    detectorDistance = "det";
    detectorAngleName = "dan.value";
  } else { // figaro
    detectorDistance = "DTR";
    detectorAngleName = "VirtualAxis.dan_actual_angle";
  }
  double detectorAngle =
      m_localWorkspace->run().getPropertyValueAsType<double>(detectorAngleName);
  auto dist = dynamic_cast<PropertyWithValue<double> *>(
      m_localWorkspace->run().getProperty(detectorDistance + ".value"));
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
