// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLTOF2.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string/predicate.hpp>

namespace {
/// An array containing the supported instrument names
const std::array<std::string, 5> SUPPORTED_INSTRUMENTS = {{"IN4", "IN5", "IN6", "PANTHER", "SHARP"}};
} // namespace

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;
using namespace HistogramData;

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLTOF2)

/**
 * Return the confidence with with this algorithm can load the file
 *
 * @param descriptor A descriptor for the file
 *
 * @return An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLTOF2::confidence(Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the ILL
  if (descriptor.pathExists("/entry0/wavelength") && descriptor.pathExists("/entry0/experiment_identifier") &&
      descriptor.pathExists("/entry0/mode") && !descriptor.pathExists("/entry0/dataSD") // This one is for
                                                                                        // LoadILLIndirect
      && !descriptor.pathExists("/entry0/instrument/VirtualChopper")                    // This one is for
                                                                                        // LoadILLReflectometry
      && !descriptor.pathExists("/entry0/data_scan")     // This one is handled by LoadILLDiffraction
      && !descriptor.pathExists("/entry0/instrument/Tx") // This eliminates SALSA data
  ) {
    return 80;
  } else {
    return 0;
  }
}

LoadILLTOF2::LoadILLTOF2() : API::IFileLoader<Kernel::NexusDescriptor>() {}

/**
 * Initialises the algorithm
 */
void LoadILLTOF2::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "File path of the Data file to load");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
  declareProperty("ConvertToTOF", false, "Convert the bin edges to time-of-flight", Direction::Input);
}

/**
 * Executes the algorithm
 */
void LoadILLTOF2::exec() {
  // Retrieve filename
  const std::string filenameData = getPropertyValue("Filename");
  bool convertToTOF = getProperty("convertToTOF");

  // open the root node
  NeXus::NXRoot dataRoot(filenameData);
  NXEntry dataFirstEntry = dataRoot.openFirstEntry();

  loadInstrumentDetails(dataFirstEntry);
  loadTimeDetails(dataFirstEntry);

  const auto monitorList = getMonitorInfo(dataFirstEntry);

  initWorkspace(dataFirstEntry);

  addAllNexusFieldsAsProperties(filenameData);
  addFacility();

  // load the instrument from the IDF if it exists
  LoadHelper::loadEmptyInstrument(m_localWorkspace, m_instrumentName);

  loadDataIntoWorkspace(dataFirstEntry, monitorList, convertToTOF);

  addEnergyToRun();
  addPulseInterval();

  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Finds monitor data names and stores them in a vector
 *
 * @param firstEntry The NeXus entry
 *
 * @return List of monitor data
 */
std::vector<std::string> LoadILLTOF2::getMonitorInfo(const NeXus::NXEntry &firstEntry) {
  std::vector<std::string> monitorList;
  for (std::vector<NXClassInfo>::const_iterator it = firstEntry.groups().begin(); it != firstEntry.groups().end();
       ++it) {
    if (it->nxclass == "NXmonitor" || boost::starts_with(it->nxname, "monitor")) {
      monitorList.push_back(it->nxname + "/data");
    }
  }
  m_numberOfMonitors = monitorList.size();
  return monitorList;
}

/**
 * Sets the instrument name along with its path in the nexus file
 *
 * @param firstEntry The NeXus entry
 */
void LoadILLTOF2::loadInstrumentDetails(const NeXus::NXEntry &firstEntry) {

  m_instrumentPath = LoadHelper::findInstrumentNexusPath(firstEntry);

  if (m_instrumentPath.empty()) {
    throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
  }

  m_instrumentName = LoadHelper::getStringFromNexusPath(firstEntry, m_instrumentPath + "/name");

  if (std::find(SUPPORTED_INSTRUMENTS.begin(), SUPPORTED_INSTRUMENTS.end(), m_instrumentName) ==
      SUPPORTED_INSTRUMENTS.end()) {
    std::string message = "The instrument " + m_instrumentName + " is not valid for this loader!";
    throw std::runtime_error(message);
  }

  // Monitor can be monitor (IN5, PANTHER) or monitor1 (IN6)
  if (firstEntry.containsGroup("monitor"))
    m_monitorName = "monitor";
  else if (firstEntry.containsGroup("monitor1"))
    m_monitorName = "monitor1";
  else {
    std::string message("Cannot find monitor[1] in the Nexus file!");
    throw std::runtime_error(message);
  }

  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param entry The NeXus entry
 * @param monitors List of monitor data
 */
void LoadILLTOF2::initWorkspace(NeXus::NXEntry &entry) {

  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  m_numberOfChannels = static_cast<size_t>(data.dim2());

  /**
   * IN4 : Rosace detector is in a different field.
   */
  size_t numberOfTubesInRosace = 0;
  if (m_instrumentName == "IN4") {
    NXData dataGroupRosace = entry.openNXData("instrument/Detector_Rosace/data");
    NXInt dataRosace = dataGroupRosace.openIntData();
    numberOfTubesInRosace += static_cast<size_t>(dataRosace.dim0());
  }

  // dim0 * m_numberOfPixelsPerTube is the total number of detectors
  m_numberOfHistograms = (m_numberOfTubes + numberOfTubesInRosace) * m_numberOfPixelsPerTube;

  g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << '\n';
  g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << '\n';
  g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << '\n';

  // Now create the output workspace
  // total number of spectra + number of monitors,
  // bin boundaries = m_numberOfChannels + 1
  // Z/time dimension
  m_localWorkspace = WorkspaceFactory::Instance().create("Workspace2D", m_numberOfHistograms + m_numberOfMonitors,
                                                         m_numberOfChannels + 1, m_numberOfChannels);

  NXClass monitor = entry.openNXGroup(m_monitorName);
  if (monitor.containsDataSet("time_of_flight")) {
    m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    m_localWorkspace->setYUnitLabel("Counts");
  } else {
    g_log.debug("PANTHER diffraction mode");
    m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    m_localWorkspace->setYUnitLabel("Counts");
  }
}

/**
 * Load the time details from the nexus file.
 *
 * @param entry :: The Nexus entry
 */
void LoadILLTOF2::loadTimeDetails(const NeXus::NXEntry &entry) {

  m_wavelength = entry.getFloat("wavelength");

  NeXus::NXClass monitorEntry = entry.openNXGroup(m_monitorName);

  if (monitorEntry.containsDataSet("time_of_flight")) {

    NXFloat time_of_flight_data = entry.openNXFloat(m_monitorName + "/time_of_flight");
    time_of_flight_data.load();

    // The entry "monitor/time_of_flight", has 3 fields:
    // channel width , number of channels, Time of flight delay
    m_channelWidth = time_of_flight_data[0];
    m_timeOfFlightDelay = time_of_flight_data[2];

    g_log.debug("Nexus Data:");
    g_log.debug() << " ChannelWidth: " << m_channelWidth << '\n';
    g_log.debug() << " TimeOfFlightDelay: " << m_timeOfFlightDelay << '\n';
    g_log.debug() << " Wavelength: " << m_wavelength << '\n';
  } // the other case is the diffraction mode for PANTHER, where nothing is
    // needed here
}

/**
 * Goes through all the fields of the NeXus file and adds them
 * as parameters in the workspace
 *
 * @param filename The NeXus file
 */
void LoadILLTOF2::addAllNexusFieldsAsProperties(const std::string &filename) {

  API::Run &runDetails = m_localWorkspace->mutableRun();

  // Open NeXus file
  NXhandle nxfileID;
  NXstatus stat = NXopen(filename.c_str(), NXACC_READ, &nxfileID);

  g_log.debug() << "Starting parsing properties from : " << filename << '\n';
  if (stat == NX_ERROR) {
    g_log.debug() << "convertNexusToProperties: Error loading " << filename;
    throw Kernel::Exception::FileError("Unable to open File:", filename);
  }
  LoadHelper::addNexusFieldsToWsRun(nxfileID, runDetails);
  NXclose(&nxfileID);
  runDetails.addProperty("run_list", runDetails.getPropertyValueAsType<int>("run_number"));
  g_log.debug() << "End parsing properties from : " << filename << '\n';
}

/**
 * Calculates the incident energy from the wavelength and adds
 * it as sample log 'Ei'
 */
void LoadILLTOF2::addEnergyToRun() {

  API::Run &runDetails = m_localWorkspace->mutableRun();
  const double ei = LoadHelper::calculateEnergy(m_wavelength);
  runDetails.addProperty<double>("Ei", ei, true); // overwrite
}

/**
 * Adds facility info to the sample logs
 */
void LoadILLTOF2::addFacility() {
  API::Run &runDetails = m_localWorkspace->mutableRun();
  runDetails.addProperty("Facility", std::string("ILL"));
}

/**
 * Calculates and adds the pulse intervals for the run
 */
void LoadILLTOF2::addPulseInterval() {
  API::Run &runDetails = m_localWorkspace->mutableRun();
  double n_pulses = -1;
  double fermiChopperSpeed = -1;

  if (m_instrumentName == "IN4") {
    fermiChopperSpeed = runDetails.getPropertyAsSingleValue("FC.rotation_speed");
    const double bkgChopper1Speed = runDetails.getPropertyAsSingleValue("BC1.rotation_speed");
    const double bkgChopper2Speed = runDetails.getPropertyAsSingleValue("BC2.rotation_speed");

    if (std::abs(bkgChopper1Speed - bkgChopper2Speed) > 1) {
      throw std::invalid_argument("Background choppers 1 and 2 have different speeds");
    }

    n_pulses = fermiChopperSpeed / bkgChopper1Speed / 4;
  } else if (m_instrumentName == "IN6") {
    fermiChopperSpeed = runDetails.getPropertyAsSingleValue("Fermi.rotation_speed");
    const double suppressorSpeed = runDetails.getPropertyAsSingleValue("Suppressor.rotation_speed");

    n_pulses = fermiChopperSpeed / suppressorSpeed;
  } else {
    return;
  }

  const double pulseInterval = 60.0 / (2 * fermiChopperSpeed) * n_pulses;
  runDetails.addProperty<double>("pulse_interval", pulseInterval);
}

/**
 * Loads all the spectra into the workspace, including that from the monitor
 *
 * @param entry The Nexus entry
 * @param monitorList Vector containing paths to monitor data
 * @param convertToTOF Should the bin edges be converted to time of flight or
 * keep the channel indexes
 */
void LoadILLTOF2::loadDataIntoWorkspace(NeXus::NXEntry &entry, const std::vector<std::string> &monitorList,
                                        bool convertToTOF) {

  g_log.debug() << "Loading data into the workspace...\n";
  // read in the data
  //  NXData dataGroup = entry.openNXData("data");
  // load the counts from the file into memory

  NXClass moni = entry.openNXGroup(m_monitorName);

  // Prepare X-axis array
  std::vector<double> xAxis(m_localWorkspace->readX(0).size());
  if (moni.containsDataSet("time_of_flight")) {
    if (convertToTOF) {
      for (size_t i = 0; i < m_numberOfChannels + 1; ++i) {
        xAxis[i] = m_timeOfFlightDelay + m_channelWidth * static_cast<double>(i) +
                   m_channelWidth / 2; // to make sure the bin centre is positive
      }
    } else {
      for (size_t i = 0; i < m_numberOfChannels + 1; ++i) {
        xAxis[i] = static_cast<double>(i); // just take the channel index
      }
    }
  } else {
    // Diffraction PANTHER
    xAxis[0] = m_wavelength * 0.9;
    xAxis[1] = m_wavelength * 1.1;
  }

  // The binning for monitors is considered the same as for detectors
  int spec = 0;
  std::vector<int> detectorIDs = m_localWorkspace->getInstrument()->getDetectorIDs(false);

  NXData dataGroup = entry.openNXData("data");
  auto data = dataGroup.openIntData();
  data.load();
  LoadHelper::fillStaticWorkspace(m_localWorkspace, data, xAxis, spec, false, detectorIDs);
  spec = static_cast<int>(m_numberOfTubes * m_numberOfPixelsPerTube);

  // IN4 Rosace detectors are in a different NeXus entry
  if (m_instrumentName == "IN4") {
    g_log.debug() << "Loading data into the workspace: IN4 Rosace!\n";
    // read in the data
    // load the counts from the file into memory
    NXData dataGroupRosace = entry.openNXData("instrument/Detector_Rosace/data");
    auto dataRosace = dataGroupRosace.openIntData();
    dataRosace.load();
    LoadHelper::fillStaticWorkspace(m_localWorkspace, dataRosace, xAxis, spec, false, detectorIDs);
    spec += dataRosace.dim0();
  }

  for (const auto &monitorName : monitorList) {
    detectorIDs[spec] = static_cast<int>(spec) + 1;
    NXData monitorGroup = entry.openNXData(monitorName);
    auto monitorData = monitorGroup.openIntData();
    monitorData.load();
    LoadHelper::fillStaticWorkspace(m_localWorkspace, monitorData, xAxis, spec, false, detectorIDs);
    spec++;
  }
}

} // namespace Mantid::DataHandling
