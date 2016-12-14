#include "MantidDataHandling/LoadILLReflectometry.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

//#include <boost/algorithm/string.hpp>
#include <algorithm>

#include <nexus/napi.h>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLReflectometry)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLReflectometry::LoadILLReflectometry()
    : m_numberOfTubes{0},         // number of tubes - X
      m_numberOfPixelsPerTube{0}, // number of pixels per tube - Y
      m_numberOfChannels{0},      // time channels - Z
      m_numberOfHistograms{0}, m_wavelength{0}, m_channelWidth{0},
      m_supportedInstruments{"D17", "d17"} {}

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
      && descriptor.pathExists("/entry0/instrument/VirtualChopper")   // ILL reflectometry
      )
    return 80;
  else
    return 0;
}

//----------------------------------------------------------------------------------------------
/** Validate inputs
 * please note, that this validation cannot be part of the confidence checking, where it would
 * also make sense.
 * @returns a string map containing the error messages
  */
std::map<std::string, std::string> LoadILLReflectometry::validateInputs() {
  std::map<std::string, std::string> result;

  const std::string fileName = getProperty("Filename");
  if (!fileName.empty() && (m_supportedInstruments.find(fileName) != m_supportedInstruments.end()))
    result["Filename"] = "Instrument not supported.";

  return result;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLReflectometry::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "File path of the Data file to load");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/**
   * Run the Child Algorithm LoadInstrument.
   */
void LoadILLReflectometry::runLoadInstrument() {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
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
  // Retrieve filename
  std::string filenameData = getPropertyValue("Filename");

  // open the root node
  NeXus::NXRoot dataRoot(filenameData);
  NXEntry firstEntry = dataRoot.openFirstEntry();

  // Load Monitor details: n. monitors x monitor contents
  std::vector<std::vector<int>> monitorsData = loadMonitors(firstEntry);

  // Load Data details (number of tubes, channels, etc)
  loadDataDetails(firstEntry);

  std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);

  initWorkspace(firstEntry, monitorsData);

  g_log.debug("Building properties...");
  loadNexusEntriesIntoProperties(filenameData);

  g_log.debug("Loading data...");
  loadData(firstEntry, monitorsData, filenameData);

  // load the instrument from the IDF if it exists
  g_log.debug("Loading instrument definition...");
  runLoadInstrument();
  placeDetector(firstEntry);

  Mantid::Kernel::NexusDescriptor descriptor(filenameData);

  // Set the channel width property
  if (descriptor.pathExists("/entry0/monitor1.time_of_flight_0")){
    auto channel_width = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty("monitor1.time_of_flight_0"));
    m_localWorkspace->mutableRun().addProperty<double>(
        "channel_width", *channel_width, true); // overwrite
  }
  else{
          g_log.information()
              << "No monitor1.time_of_flight_0 : \n";
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
  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
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
    NeXus::NXEntry & /*entry*/, std::vector<std::vector<int>> monitorsData) {

  g_log.debug() << "Number of monitors: " << monitorsData.size() << '\n';
  for (size_t i = 0; i < monitorsData.size(); ++i)
    g_log.debug() << "Data size of monitor[" << i <<"]: " << monitorsData[i].size() << '\n';

  // create the output workspace
  m_localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", m_numberOfHistograms + monitorsData.size(),
      m_numberOfChannels + 1, m_numberOfChannels);

  Mantid::Kernel::NexusDescriptor descriptor(filename);

  if (descriptor.pathExists("/entry0/monitor1.time_of_flight_0"))
    m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Load Data details (number of tubes, channels, etc)
 * @param entry First entry of nexus file
 */
void LoadILLReflectometry::loadDataDetails(NeXus::NXEntry &entry) {
  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << '\n';

  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << '\n';

  // total number of detectors
  m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;
  g_log.debug() << "Number of detectors" << m_numberOfHistograms << '\n';

  m_numberOfChannels = static_cast<size_t>(data.dim2());
  g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << '\n';
}

/**
 * Load single monitor
 *
 * @param entry :: The Nexus entry
 * @param monitor_id :: A std::string containing the Nexus path to the monitor data
 * @return monitor :: A std::vector containing monitor values
 */
std::vector<int>
LoadILLReflectometry::loadSingleMonitor(NeXus::NXEntry &entry, std::string monitor_data){

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
 * Load data found in nexus file
 *
 * @param entry :: The Nexus entry
 * @param monitorsData :: Monitors data already loaded
 *
 */
void LoadILLReflectometry::loadData(
    NeXus::NXEntry &entry, std::vector<std::vector<int>> monitorsData, std::string &filename) {

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

 Mantid::Kernel::NexusDescriptor descriptor(filename);

 std::vector<double> xVals;
 if (descriptor.pathExists("/entry0/monitor1.time_of_flight_0")){
      // get some parameters from nexus file and properties
      const std::string propTOF0 = "monitor1.time_of_flight_0";
      auto tof_channel_width_prop = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(propTOF0));
      if (!tof_channel_width_prop)
        throw std::runtime_error("Could not cast (interpret) the property " +
                                 propTOF0 + " (channel width) as a floating point "
                                            "value.");
      m_channelWidth = *tof_channel_width_prop; /* PAR1[95] */

      const std::string propTOF2 = "monitor1.time_of_flight_2";
      auto tof_delay_prop = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty(propTOF2));
      if (!tof_delay_prop)
        throw std::runtime_error("Could not cast (interpret) the property " +
                                 propTOF2 +
                                 " (ToF delay) as a floating point value.");
      double tof_delay = *tof_delay_prop; /* PAR1[96] */


      double POFF = entry.getFloat("instrument/VirtualChopper/poff"); /* par1[54] */
      double open_offset =
          entry.getFloat("instrument/VirtualChopper/open_offset"); /* par1[56] */
      double mean_chop_1_phase = 0.0;
      double mean_chop_2_phase = 0.0;
      // [30/09/14] Test on availability of VirtualChopper data
      double chop1_speed = entry.getFloat(
          "instrument/VirtualChopper/chopper1_speed_average"); /* PAR2[109] */
      if (chop1_speed != 0.0) {
        // Virtual Chopper entries are valid

        // double mean_chop_1_phase =
        // entry.getFloat("instrument/VirtualChopper/chopper1_phase_average"); /*
        // PAR2[110] */
        // this entry seems to be wrong for now, we use the old one instead [YR
        // 5/06/2014]
        mean_chop_1_phase = entry.getFloat("instrument/Chopper1/phase");
        mean_chop_2_phase = entry.getFloat(
            "instrument/VirtualChopper/chopper2_phase_average"); /* PAR2[114] */

      } else {
        // Use Chopper values instead
        chop1_speed =
            entry.getFloat("instrument/Chopper1/rotation_speed"); /* PAR2[109] */

        mean_chop_1_phase = entry.getFloat("instrument/Chopper1/phase");
        mean_chop_2_phase = entry.getFloat("instrument/Chopper2/phase");
      }

      g_log.debug() << "m_channelWidth: " << m_channelWidth << '\n';
      //g_log.debug() << "tof_delay: " << tof_delay << '\n';
      g_log.debug() << "POFF: " << POFF << '\n';
      g_log.debug() << "open_offset: " << open_offset << '\n';
      g_log.debug() << "mean_chop_2_phase: " << mean_chop_2_phase << '\n';
      g_log.debug() << "mean_chop_1_phase: " << mean_chop_1_phase << '\n';
      g_log.debug() << "chop1_speed: " << chop1_speed << '\n';

      double t_TOF2 = 0.0;
      if (chop1_speed == 0.0) {
        g_log.debug() << "Warning: chop1_speed is null.\n";
        // stay with t_TOF2 to O.0
      } else {
        // Thanks to Miguel Gonzales/ILL for this TOF formula
        t_TOF2 = -1.e6 * 60.0 * (POFF - 45.0 + mean_chop_2_phase -
                                 mean_chop_1_phase + open_offset) /
                 (2.0 * 360 * chop1_speed);
      }
      g_log.debug() << "t_TOF2: " << t_TOF2 << '\n';

      // compute tof values
      xVals.reserve(m_localWorkspace->x(0).size());

      for (size_t timechannelnumber = 0; timechannelnumber <= m_numberOfChannels;
           ++timechannelnumber) {
        double t_TOF1 =
            (static_cast<int>(timechannelnumber) + 0.5) * m_channelWidth +
            tof_delay;
        xVals.push_back(t_TOF1 + t_TOF2);
      }
  }
  else{
          g_log.information()
              << "No monitorx.time_of_flight_x \n";

          xVals.reserve(m_localWorkspace->x(0).size());
          for (size_t t = 0; t <= m_numberOfChannels;
               ++t) {
            double dt = double(t);
            xVals.push_back(dt);
          }

  }

  HistogramData::BinEdges binEdges(xVals);

  // Write monitors
  for (size_t im = 0; im < nb_monitors; im++) {
    int *monitor_p = monitorsData[im].data();
    const HistogramData::Counts histoCounts(monitor_p,
                                            monitor_p + m_numberOfChannels);
    const HistogramData::CountStandardDeviations histoBlankError(
        monitorsData[im].size(), 0.0);
    m_localWorkspace->setHistogram(im, binEdges, std::move(histoCounts),
                                   std::move(histoBlankError));

    progress.report();
  }

  // Write data
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

} // LoadILLIndirect::loadDataIntoTheWorkSpace

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

  // Add also "Facility", as asked
  runDetails.addProperty("Facility", std::string("ILL"));

  stat = NXclose(&nxfileID);
}

/**
 * Utility to center detector.
 */
/*
void LoadILLReflectometry::centerDetector(double xCenter) {

  std::string componentName("uniq_detector");
  V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
  // TODO confirm!
  pos.setX(pos.X() - xCenter);
  m_loader.moveComponent(m_localWorkspace, componentName, pos);
}
*/

/**
 * Utility to place detector in space, according to data file
 *
 * @param entry :: The first entry of the Nexus file
 */
void LoadILLReflectometry::placeDetector(NeXus::NXEntry &entry) {
    // Get distance and tilt angle stored in nexus file
    // Mantid way
    ////	auto angleProp =
    /// dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("dan.value"));
    // Nexus way
    double angle =
        entry.getFloat("instrument/dan/value"); // detector angle in degrees
    double distance = entry.getFloat(
        "instrument/det/value"); // detector distance in millimeter

    distance /= 1000.0; // convert to meter
    g_log.debug() << "Moving detector at angle " << angle << " and distance "
                  << distance << '\n';

  const double deg2rad = M_PI / 180.0;
  std::string componentName("uniq_detector");
  V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
  //      double r, theta, phi;
  //      pos.getSpherical(r, theta, phi);

  double angle_rad = angle * deg2rad;
  V3D newpos(distance * sin(angle_rad), pos.Y(), distance * cos(angle_rad));
  m_loader.moveComponent(m_localWorkspace, componentName, newpos);

  // Apply a local rotation to stay perpendicular to the beam
  const V3D axis(0.0, 1.0, 0.0);
  Quat rotation(angle, axis);
  m_loader.rotateComponent(m_localWorkspace, componentName, rotation);
}
} // namespace DataHandling
} // namespace Mantid
