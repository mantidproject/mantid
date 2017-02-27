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

//#include <algorithm>
//#include <nexus/napi.h>

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
  if (descriptor.pathExists("/entry0/wavelength")               // ILL
      && descriptor.pathExists("/entry0/experiment_identifier") // ILL
      && descriptor.pathExists("/entry0/mode")                  // ILL
      &&
      descriptor.pathExists(
          "/entry0/instrument/VirtualChopper") // ILL reflectometry
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

  const std::vector<std::string> theta{"san", "dan", "theta"};
  declareProperty(
      "Theta", "san", boost::make_shared<StringListValidator>(theta),
      "Optional angle for calculating the scattering angle.\n"
      "San (sample angle), dan (detector angle), theta (user defined angle)");

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(1.0);
  declareProperty("ThetaUserDefined", EMPTY_DBL(), positiveDouble,
                  "For selected Theta equals theta, the user must provide an "
                  "angle in degree");

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
  const std::string fileName = getProperty("Filename");
  if (!fileName.empty() &&
      (m_supportedInstruments.find(fileName) != m_supportedInstruments.end()))
    result["Filename"] = "Instrument not supported.";
  // check user defined angle
  const double thetaUserDefined{getProperty("ThetaUserDefined")};
  const std::string angleOption = getProperty("Theta");
  if ((angleOption == "theta") && (thetaUserDefined == EMPTY_DBL()))
    result["ThetaUserDefined"] =
        "User defined theta option requires an input value";
  if ((angleOption != "theta") && (thetaUserDefined != EMPTY_DBL()))
    result["ThetaUserDefined"] =
        "No input value required for user defined theta option";
  // check direct beam file
  const std::string directBeam = getProperty("DirectBeam");
  if (!directBeam.empty() &&
      (m_supportedInstruments.find(directBeam) != m_supportedInstruments.end()))
    result["DirectBeam"] = "Instrument not supported.";

  return result;
}

//----------------------------------------------------------------------------------------------
/**
   * Run the Child Algorithm LoadInstrument.
   */
void LoadILLReflectometry::runLoadInstrument() {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // execute the Child Algorithm. Catch and log any error, but don't stop.
  try {

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

  // load Data details (number of tubes, channels, etc)
  loadDataDetails(firstEntry);

  std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);

  initWorkspace(firstEntry, monitorsData, filenameData);

  g_log.debug("Building properties...");
  loadNexusEntriesIntoProperties(filenameData);

  // load the instrument from the IDF if it exists
  g_log.debug("Loading instrument definition...");
  runLoadInstrument();
  placeDetector();

  Mantid::Kernel::NexusDescriptor descriptor(filenameData);

  std::vector<double> xVals;
  // Set the channel width property and get x values
  if (descriptor.pathExists("/entry0/monitor1/time_of_flight")) {
    auto channel_width = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(
            "monitor1.time_of_flight_0")); /* PAR1[95] */
    m_localWorkspace->mutableRun().addProperty<double>(
        "channel_width", *channel_width, true); // overwrite
    m_channelWidth = *channel_width;
    getXValues(xVals);
  } else {
    g_log.debug() << "Did not find Nexus entry monitor1.time_of_flight \n";
    m_channelWidth = 0;

    g_log.debug() << "No monitor TOF values for axis description (no "
                     "conversion to wavelength possible). \n";

    xVals.reserve(m_localWorkspace->x(0).size());
    for (size_t t = 0; t <= m_numberOfChannels; ++t) {
      double dt = double(t);
      xVals.push_back(dt);
    }
  }
  g_log.debug() << "Channel width: " << m_channelWidth << '\n';
  g_log.debug("Loading data...");
  loadData(firstEntry, monitorsData, xVals);
  const std::string unit = getPropertyValue("XUnit");
  if (descriptor.pathExists("/entry0/monitor1/time_of_flight") &&
      (unit == "Wavelength")) {
    auto convertToWavelength = createChildAlgorithm("ConvertUnits", true);
    convertToWavelength->initialize();
    convertToWavelength->setLogging(true);
    convertToWavelength->enableHistoryRecordingForChild(true);
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                                           m_localWorkspace);
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                                           m_localWorkspace);
    convertToWavelength->setPropertyValue("Target", "Wavelength");
    convertToWavelength->setProperty("AlignBins", true);
    convertToWavelength->execute();
  }
  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
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
  boost::to_upper(m_instrumentName); // "D17" in file, keep it upper case.
  g_log.debug() << "Instrument name : " + m_instrumentName << '\n';
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param entry :: The Nexus entry
 * @param monitorsData :: Monitors data already loaded
 *
 */
void LoadILLReflectometry::initWorkspace(
    NeXus::NXEntry & /*entry*/, std::vector<std::vector<int>> monitorsData,
    const std::string &filename) {

  g_log.debug() << "Number of monitors: " << monitorsData.size() << '\n';
  for (size_t i = 0; i < monitorsData.size(); ++i)
    g_log.debug() << "Data size of monitor" << i << ": "
                  << monitorsData[i].size() << '\n';

  // create the output workspace
  m_localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", m_numberOfHistograms + monitorsData.size(),
      m_numberOfChannels + 1, m_numberOfChannels);

  Mantid::Kernel::NexusDescriptor descriptor(filename);

  if (descriptor.pathExists("/entry0/monitor1/time_of_flight"))
    m_localWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create("TOF");

  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Load Data details (number of tubes, channels, etc)
 * @param entry First entry of nexus file
 */
void LoadILLReflectometry::loadDataDetails(NeXus::NXEntry &entry) {
  // read in the data
  NXData dataGroup{entry.openNXData("data")};
  NXInt data{dataGroup.openIntData()};

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  g_log.debug() << "Number of tubes: " << m_numberOfTubes << '\n';

  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  g_log.debug() << "Number of pixels per tube: " << m_numberOfPixelsPerTube
                << '\n';

  m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;
  g_log.debug() << "Number of detectors: " << m_numberOfHistograms << '\n';

  m_numberOfChannels = static_cast<size_t>(data.dim2());
  g_log.debug() << "Number of time channels: " << m_numberOfChannels << '\n';
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
 *
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
 *
 */
void LoadILLReflectometry::getXValues(std::vector<double> &xVals) {
  try {
    // use Chopper1.phase
    auto chop1_phase = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty("Chopper1.phase"));

    auto tof_1 = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty("monitor1.time_of_flight_2"));

    auto POFF = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty("VirtualChopper.poff"));

    auto open_offset = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty("VirtualChopper.open_offset"));

    auto chop1_speed = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(
            "VirtualChopper.chopper1_speed_average"));

    auto chop2_speed = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(
            "VirtualChopper.chopper2_speed_average"));

    auto chop2_phase = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty(
            "VirtualChopper.chopper2_phase_average"));

    std::string chopper;

    if (*chop1_speed && *chop2_speed && *chop2_phase) {
      // virtual Chopper entries are valid
      chopper = "Virtual chopper";
    } else {
      // use Chopper values instead

      chop1_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("Chopper1.rotation_speed"));

      chop2_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("Chopper2.rotation_speed"));

      chop2_phase = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("Chopper2.phase"));

      chopper = "Chopper";
    }

    // major logging
    g_log.debug() << "TOF delay: " << *tof_1 << '\n';
    g_log.debug() << "Poff: " << *POFF << '\n';
    g_log.debug() << "Open offset: " << *open_offset << '\n';
    g_log.debug() << "Chopper 1 phase : " << *chop1_phase << '\n';
    g_log.debug() << chopper << " 1 speed : " << *chop1_speed << '\n';
    g_log.debug() << chopper << " 2 phase : " << *chop2_phase << '\n';
    g_log.debug() << chopper << " 2 speed : " << *chop2_speed << '\n';

    double t_TOF2 = 0.0;
    if (!chop1_speed) {
      g_log.debug() << "Warning: chop1_speed is null.\n";
    } else {
      // thanks to Miguel Gonzales/ILL for this TOF formula
      t_TOF2 = -1.e+6 * 60.0 *
               (*POFF - 45.0 + *chop2_phase - *chop1_phase + *open_offset) /
               (2.0 * 360 * *chop1_speed);
    }
    g_log.debug() << "t_TOF2 : " << t_TOF2 << '\n';

    // compute tof values
    xVals.reserve(m_localWorkspace->x(0).size());

    for (size_t timechannelnumber = 0; timechannelnumber <= m_numberOfChannels;
         ++timechannelnumber) {
      double t_TOF1 =
          (static_cast<int>(timechannelnumber) + 0.5) * m_channelWidth + *tof_1;
      xVals.push_back(t_TOF1 + t_TOF2);
    }
  } catch (std::runtime_error &e) {
    g_log.information() << "Unable to access Nexus file entries : " << e.what()
                        << '\n';
  }
}

/**
 * Load data from nexus file
 *
 * @param entry :: The Nexus file entry
 * @param monitorsData :: Monitors data already loaded
 * @param xVals :: X values
 *
 */
void LoadILLReflectometry::loadData(NeXus::NXEntry &entry,
                                    std::vector<std::vector<int>> monitorsData,
                                    std::vector<double> &xVals) {

  m_wavelength = entry.getFloat("wavelength");

  double ei = m_loader.calculateEnergy(m_wavelength);
  m_localWorkspace->mutableRun().addProperty<double>("Ei", ei,
                                                     true); // overwrite

  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();

  size_t nb_monitors = monitorsData.size();

  Progress progress(this, 0, 1,
                    m_numberOfTubes * m_numberOfPixelsPerTube + nb_monitors);

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
  for (size_t i = 0; i < m_numberOfTubes; ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      const HistogramData::Counts histoCounts(data_p,
                                              data_p + m_numberOfChannels);
      m_localWorkspace->setHistogram((spec + nb_monitors), binEdges,
                                     std::move(histoCounts));
      ++spec;
      progress.report();
    }
  }

} // LoadILLIndirect::loadData

/**
 * Use the LoadHelper utility to load most of the nexus entries into workspace
 * log properties
 */
void LoadILLReflectometry::loadNexusEntriesIntoProperties(
    std::string nexusfilename) {

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

  // Add also "Facility"
  runDetails.addProperty("Facility", std::string("ILL"));

  stat = NXclose(&nxfileID);
}

/**
 * Utility to place detector in space, according to data file
 */
void LoadILLReflectometry::placeDetector() {
  g_log.debug() << "Move the detector bank \n";
  std::string thetaIn = getPropertyValue("Theta");
  double theta = getProperty("ThetaUserDefined");
  if (theta == EMPTY_DBL()) {
    const std::string name = thetaIn + ".value";
    if (m_localWorkspace->run().hasProperty(name)) {
      theta = m_localWorkspace->run().getPropertyValueAsType<double>(name);
    } else {
      throw std::runtime_error("Theta is not defined");
    }
  }
  g_log.debug() << "2Theta " << 2. * theta << " degrees\n";
  double twotheta_rad = (2. * theta) * M_PI / 180.0;
  // incident theta angle for being able to call the algorithm
  // ConvertToReflectometryQ
  m_localWorkspace->mutableRun().addProperty("stheta",
                                             double(twotheta_rad / 2.));
  auto dist = dynamic_cast<PropertyWithValue<double> *>(
      m_localWorkspace->run().getProperty("det.value"));
  double distance = *dist / 1000.0; // convert to meter
  g_log.debug() << "Sample - detector distance in millimeter " << *dist << '\n';

  /*
  Lamp

  conversion to radians *pi/180
  conversion to grad    *180/pi

  parref: parameters from reflected beam

  bragg angle (rad)

  ============================================================================================

  parref.san in grad

  san coherent
  --------------------------------------------------------------------------------------------

  angle_bragg = (parref.san * !pi / 180.) -
                0.5*atan((float(peakref)-c_params.pcen)*c_params.pixelwidth/parref.sdetd)
  +
                0.5*atan((newpeakref-c_params.pcen)*c_params.pixelwidth/parref.sdetd)

  san incoherent
  --------------------------------------------------------------------------------------------

  angle_bragg = parref.san * !pi / 180.

  ============================================================================================

  dan     Direct peak position (new from gaussian fit) = newpeakdir
  dan     Reflect peak position (new from gaussian fit) = newpeakref
  angle_centre = ((parref.dan - pardir.dan) / 2.) * !pi / 180.

  dan coherent
  --------------------------------------------------------------------------------------------

  detector pixels projected onto a given pixel, so we need
  to select middle of specular pixel (peakref) instead of value from fit

  angle_bragg = angle_centre +
                0.5*atan((newpeakdir-c_params.pcen)*c_params.pixelwidth/pardir.sdetd)
  -
                0.5*atan(((float(peakref)+0.5)-c_params.pcen)*c_params.pixelwidth/parref.sdetd)
                we have to use the middle of the coherent center pixel

  dan incoherent
  --------------------------------------------------------------------------------------------

  angle_bragg = angle_centre +
                0.5*atan((newpeakdir-c_params.pcen)*c_params.pixelwidth/pardir.sdetd)
  -
                0.5*atan((newpeakref-c_params.pcen)*c_params.pixelwidth/parref.sdetd)

  ============================================================================================

    - user defined using theta value from table

  ============================================================================================

  twotheta = 2.0 * angle_bragg (rad)


  ; calculate angle offset for flat detector surface
  function cosmos_anal_calcangleoffset, peakpos, xoff, density, distance
  return  atan(((peakpos - xoff) * density - (c_params.pixels_x / 2.)-0.5) *
  c_params.pixelwidth, distance)

  ; calculate corrected time of flight distance for flat detector surface
  cosmos_anal_correctdistance(peakpos, xoff, density, distance)
  return distance / cos(cosmos_anal_calcangleoffset(peakpos, xoff, density,
  distance))

  calculate lambda (wavelength in angstroms) determined for reflected beam only

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

  const std::string componentName = "bank";
  try {
    V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
    V3D newpos(distance * sin(twotheta_rad), pos.Y(),
               distance * cos(twotheta_rad));
    m_loader.moveComponent(m_localWorkspace, componentName, newpos);
    // apply a local rotation to stay perpendicular to the beam
    const V3D axis(0.0, 1.0, 0.0);
    Quat rotation(2. * theta, axis);
    m_loader.rotateComponent(m_localWorkspace, componentName, rotation);
  } catch (std::runtime_error &e) {
    throw std::runtime_error("Unable to move D17 " + componentName +
                             " of the instrument definition file " + e.what() +
                             '\n');
  }
}
} // namespace DataHandling
} // namespace Mantid
