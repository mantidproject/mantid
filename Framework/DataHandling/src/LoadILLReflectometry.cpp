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

  initWorkspace(firstEntry, monitorsData, filenameData);

  g_log.debug("Building properties...");
  loadNexusEntriesIntoProperties(filenameData);

  g_log.debug("Loading data...");
  loadData(firstEntry, monitorsData);

  // load the instrument from the IDF if it exists
  g_log.debug("Loading instrument definition...");
  runLoadInstrument();
  placeDetector();

  Mantid::Kernel::NexusDescriptor descriptor(filenameData);

  // Set the channel width property
  if (descriptor.pathExists("/entry0/monitor1/time_of_flight")){
    auto channel_width = dynamic_cast<PropertyWithValue<double> *>(
        m_localWorkspace->run().getProperty("monitor1.time_of_flight_0"));
    m_localWorkspace->mutableRun().addProperty<double>(
        "channel_width", *channel_width, true); // overwrite
  }
  else{
    g_log.debug() << "Did not find Nexus entry monitor1.time_of_flight \n";
  }

  // convert to wavelength using algorithm ConvertUnits
  auto convertToWavelength = createChildAlgorithm("ConvertUnits", true);
  convertToWavelength->initialize();
  convertToWavelength->setLogging(true);
  convertToWavelength->enableHistoryRecordingForChild(true);

  // execute ConvertUnits
  if (descriptor.pathExists("/entry0/monitor1/time_of_flight")){

    convertToWavelength->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_localWorkspace);
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", m_localWorkspace);
    convertToWavelength->setPropertyValue("Target", "Wavelength");
    convertToWavelength->setPropertyValue("EMode", "Direct");
    //convertToWavelength->setProperty<bool>("AlignBins", true); // very critical to use, does not work anyways, seems to be a bug
    //convertToWavelength->setProperty<bool>("ConvertFromPointData", false);

    convertToWavelength->execute();
  }
  else
    g_log.debug() << "No wavelength axis description. \n";

  // transpose workspace (histogram)

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
    NeXus::NXEntry & /*entry*/, std::vector<std::vector<int>> monitorsData, std::string &filename) {

  g_log.debug() << "Number of monitors: " << monitorsData.size() << '\n';
  for (size_t i = 0; i < monitorsData.size(); ++i)
    g_log.debug() << "Data size of monitor" << i <<": " << monitorsData[i].size() << '\n';

  // create the output workspace
  m_localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", m_numberOfHistograms + monitorsData.size(),
      m_numberOfChannels + 1, m_numberOfChannels);

  Mantid::Kernel::NexusDescriptor descriptor(filename);

  if (descriptor.pathExists("/entry0/monitor1/time_of_flight"))
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
  g_log.debug() << "Number of tubes: " << m_numberOfTubes << '\n';

  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  g_log.debug() << "Number of pixels per tube: " << m_numberOfPixelsPerTube << '\n';

  m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;
  g_log.debug() << "Number of detectors: " << m_numberOfHistograms << '\n';

  m_numberOfChannels = static_cast<size_t>(data.dim2());
  g_log.debug() << "Number of time channels: " << m_numberOfChannels << '\n';
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
    NeXus::NXEntry &entry, std::vector<std::vector<int>> monitorsData) {

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

  std::vector<double> xVals;

  // unknown property search object, readd check for Nexus file entry

  auto tof_0 = dynamic_cast<PropertyWithValue<double> *>(
    m_localWorkspace->run().getProperty("monitor1.time_of_flight_0")); /* PAR1[95] */

  auto tof_1 = dynamic_cast<PropertyWithValue<double> *>(
    m_localWorkspace->run().getProperty("monitor1.time_of_flight_2")); /* PAR1[96] */

  auto POFF = dynamic_cast<PropertyWithValue<double> *>(
    m_localWorkspace->run().getProperty("VirtualChopper.poff")); /* par1[54] */

  auto open_offset = dynamic_cast<PropertyWithValue<double> *>(
    m_localWorkspace->run().getProperty("VirtualChopper.open_offset")); /* par1[56] */

  if (tof_0 && tof_1 && POFF && open_offset){

      m_channelWidth = *tof_0;
      g_log.debug() << "Channel width: " << m_channelWidth << '\n';
      g_log.debug() << "TOF delay: " << *tof_1 << '\n';
      g_log.debug() << "Poff: " << *POFF << '\n';
      g_log.debug() << "Open offset: " << *open_offset << '\n';

      // test on availability of VirtualChopper data
      auto chop1_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("VirtualChopper.chopper1_speed_average")); /* PAR2[109] */

      auto chop2_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("VirtualChopper.chopper2_speed_average")); /* PAR2[109] */

      auto chop1_phase = dynamic_cast<PropertyWithValue<double> *>(
         m_localWorkspace->run().getProperty("VirtualChopper.chopper1_phase_average"));

      auto chop2_phase = dynamic_cast<PropertyWithValue<double> *>(
         m_localWorkspace->run().getProperty("VirtualChopper.chopper2_phase_average")); /* PAR2[114] */

      std::string chopper;

      if (chop1_speed && chop2_speed && chop1_phase && chop2_phase) {
        // virtual Chopper entries are valid
        chopper = "Virtual chopper";
      } else {// Lamp seems to use chopper only and phases divided by 100?
        // use Chopper values instead

        chop1_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("Chopper1.rotation_speed")); /* PAR2[109] */

        chop2_speed = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("Chopper2.rotation_speed")); /* PAR2[109] */

        chop1_phase = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("Chopper1.phase"));

        chop2_phase = dynamic_cast<PropertyWithValue<double> *>(
          m_localWorkspace->run().getProperty("Chopper2.phase"));

        chopper = "Chopper";
      }

      g_log.debug() << chopper << " 1 phase : " << *chop1_phase << '\n';
      g_log.debug() << chopper << " 1 speed : " << *chop1_speed << '\n';
      g_log.debug() << chopper << " 2 phase : " << *chop2_phase << '\n';
      g_log.debug() << chopper << " 2 speed : " << *chop2_speed << '\n';

      double t_TOF2 = 0.0;
      if (!chop1_speed) {
        g_log.debug() << "Warning: chop1_speed is null.\n";
      } else {
        // Thanks to Miguel Gonzales/ILL for this TOF formula
        t_TOF2 = -1.e6 * 60.0 * (*POFF - 45.0 + *chop2_phase -
                                 *chop1_phase + *open_offset) /
                 ( 2 * 360 * *chop1_speed);
      }
      g_log.debug() << "t_TOF2: " << t_TOF2 << '\n';

      // compute tof values
      xVals.reserve(m_localWorkspace->x(0).size());

      for (size_t timechannelnumber = 0; timechannelnumber <= m_numberOfChannels;
           ++timechannelnumber) {
        double t_TOF1 =
            (static_cast<int>(timechannelnumber) + 0.5) * m_channelWidth +
            *tof_1;
        xVals.push_back(t_TOF1 + t_TOF2);

     }
  }
  else{
    g_log.debug() << "No monitor TOF values for axis description. \n";

    xVals.reserve(m_localWorkspace->x(0).size());
    for (size_t t = 0; t <= m_numberOfChannels; ++t) {
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

  // Add also "Facility", as asked
  runDetails.addProperty("Facility", std::string("ILL"));

  stat = NXclose(&nxfileID);
}

/**
 * Utility to place detector in space, according to data file
 *
 * @param entry :: The first entry of the Nexus file
 */
void LoadILLReflectometry::placeDetector() {

    auto angle =
     dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("dan.value"));
    g_log.debug() << "Detector angle in degree "
                  << angle->value() << '\n';

    auto sample_angle =
     dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("san.value"));
    g_log.debug() << "Sample angle in degree "
                  << sample_angle->value() << '\n';


    g_log.debug() << "Moving \n";

    auto dist =
     dynamic_cast<PropertyWithValue<double>*>(m_localWorkspace->run().getProperty("det.value"));
    double distance = *dist / 1000.0; // convert to meter
    g_log.debug() << "Sample - detector distance in meter "
                  << *dist << '\n';



  std::string componentName("uniq_detector");
  V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
  //      double r, theta, phi;
  //      pos.getSpherical(r, theta, phi);

  // center detector?
  //pos.setX(pos.X() - xCenter);

  double angle_rad = *angle * M_PI / 180.0;
  V3D newpos(distance * sin(angle_rad), pos.Y(), distance * cos(angle_rad));
  m_loader.moveComponent(m_localWorkspace, componentName, newpos);

  // apply a local rotation to stay perpendicular to the beam
  const V3D axis(0.0, 1.0, 0.0);
  Quat rotation(*angle, axis);
  m_loader.rotateComponent(m_localWorkspace, componentName, rotation);

  g_log.debug() << "End moving.\n";
}
} // namespace DataHandling
} // namespace Mantid
