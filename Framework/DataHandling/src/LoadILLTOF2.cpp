// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLTOF2.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string/predicate.hpp>

namespace {
/// An array containing the supported instrument names
const std::array<std::string, 4> SUPPORTED_INSTRUMENTS = {
    {"IN4", "IN5", "IN6", "PANTHER"}};
} // namespace

namespace Mantid {
namespace DataHandling {

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
  if (descriptor.pathExists("/entry0/wavelength") &&
      descriptor.pathExists("/entry0/experiment_identifier") &&
      descriptor.pathExists("/entry0/mode") &&
      !descriptor.pathExists("/entry0/dataSD") // This one is for
                                               // LoadILLIndirect
      && !descriptor.pathExists(
             "/entry0/instrument/VirtualChopper") // This one is for
                                                  // LoadILLReflectometry
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
  declareProperty(std::make_unique<FileProperty>("Filename", "",
                                                 FileProperty::Load, ".nxs"),
                  "File path of the Data file to load");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to use for the output workspace");
}

/**
 * Executes the algorithm
 */
void LoadILLTOF2::exec() {
  // Retrieve filename
  const std::string filenameData = getPropertyValue("Filename");

  // open the root node
  NeXus::NXRoot dataRoot(filenameData);
  NXEntry dataFirstEntry = dataRoot.openFirstEntry();

  loadInstrumentDetails(dataFirstEntry);
  loadTimeDetails(dataFirstEntry);

  const auto monitors = getMonitorInfo(dataFirstEntry);

  initWorkSpace(dataFirstEntry, monitors);

  addAllNexusFieldsAsProperties(filenameData);
  addFacility();

  runLoadInstrument(); // just to get IDF contents

  loadDataIntoTheWorkSpace(dataFirstEntry, monitors);

  addEnergyToRun();
  addPulseInterval();

  // load the instrument from the IDF if it exists
  runLoadInstrument();

  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Loads Monitor data into an vector of monitor data
 *
 * @param firstEntry The NeXus entry
 *
 * @return List of monitor data
 */
std::vector<std::vector<int>>
LoadILLTOF2::getMonitorInfo(NeXus::NXEntry &firstEntry) {

  std::vector<std::vector<int>> monitorList;

  for (std::vector<NXClassInfo>::const_iterator it =
           firstEntry.groups().begin();
       it != firstEntry.groups().end(); ++it) {

    if (it->nxclass == "NXmonitor" ||
        boost::starts_with(it->nxname, "monitor")) {

      g_log.debug() << "Load monitor data from " + it->nxname;

      NXData dataGroup = firstEntry.openNXData(it->nxname + "/data");
      NXInt data = dataGroup.openIntData();
      // load the counts from the file into memory
      data.load();

      std::vector<int> thisMonitor(data(), data() + data.size());
      monitorList.push_back(thisMonitor);
    }
  }
  return monitorList;
}

/**
 * Sets the instrument name along with its path in the nexus file
 *
 * @param firstEntry The NeXus entry
 */
void LoadILLTOF2::loadInstrumentDetails(NeXus::NXEntry &firstEntry) {

  m_instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);

  if (m_instrumentPath.empty()) {
    throw std::runtime_error(
        "Cannot set the instrument name from the Nexus file!");
  }

  m_instrumentName =
      m_loader.getStringFromNexusPath(firstEntry, m_instrumentPath + "/name");

  if (std::find(SUPPORTED_INSTRUMENTS.begin(), SUPPORTED_INSTRUMENTS.end(),
                m_instrumentName) == SUPPORTED_INSTRUMENTS.end()) {
    std::string message =
        "The instrument " + m_instrumentName + " is not valid for this loader!";
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
void LoadILLTOF2::initWorkSpace(NeXus::NXEntry &entry,
                                const std::vector<std::vector<int>> &monitors) {

  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  m_numberOfChannels = static_cast<size_t>(data.dim2());
  const size_t numberOfMonitors = monitors.size();

  /**
   * IN4 : Rosace detector is in a different field.
   */
  size_t numberOfTubesInRosace = 0;
  if (m_instrumentName == "IN4") {
    NXData dataGroupRosace =
        entry.openNXData("instrument/Detector_Rosace/data");
    NXInt dataRosace = dataGroupRosace.openIntData();
    numberOfTubesInRosace += static_cast<size_t>(dataRosace.dim0());
  }

  // dim0 * m_numberOfPixelsPerTube is the total number of detectors
  m_numberOfHistograms =
      (m_numberOfTubes + numberOfTubesInRosace) * m_numberOfPixelsPerTube;

  g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << '\n';
  g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << '\n';
  g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << '\n';

  // Now create the output workspace
  // total number of spectra + number of monitors,
  // bin boundaries = m_numberOfChannels + 1
  // Z/time dimension
  m_localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", m_numberOfHistograms + numberOfMonitors,
      m_numberOfChannels + 1, m_numberOfChannels);
  m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Load the time details from the nexus file.
 *
 * @param entry :: The Nexus entry
 */
void LoadILLTOF2::loadTimeDetails(NeXus::NXEntry &entry) {

  m_wavelength = entry.getFloat("wavelength");

  // Monitor can be monitor (IN5) or monitor1 (IN6)
  std::string monitorName;
  if (entry.containsGroup("monitor"))
    monitorName = "monitor";
  else if (entry.containsGroup("monitor1"))
    monitorName = "monitor1";
  else {
    std::string message("Cannot find monitor[1] in the Nexus file!");
    g_log.error(message);
    throw std::runtime_error(message);
  }

  NXFloat time_of_flight_data =
      entry.openNXFloat(monitorName + "/time_of_flight");
  time_of_flight_data.load();

  // The entry "monitor/time_of_flight", has 3 fields:
  // channel width , number of channels, Time of flight delay
  m_channelWidth = time_of_flight_data[0];
  m_timeOfFlightDelay = time_of_flight_data[2];

  g_log.debug("Nexus Data:");
  g_log.debug() << " ChannelWidth: " << m_channelWidth << '\n';
  g_log.debug() << " TimeOfFlightDealy: " << m_timeOfFlightDelay << '\n';
  g_log.debug() << " Wavelength: " << m_wavelength << '\n';
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
  m_loader.addNexusFieldsToWsRun(nxfileID, runDetails);
  NXclose(&nxfileID);
  g_log.debug() << "End parsing properties from : " << filename << '\n';
}

/**
 * Calculates the incident energy from the wavelength and adds
 * it as sample log 'Ei'
 */
void LoadILLTOF2::addEnergyToRun() {

  API::Run &runDetails = m_localWorkspace->mutableRun();
  const double ei = m_loader.calculateEnergy(m_wavelength);
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
    fermiChopperSpeed =
        runDetails.getPropertyAsSingleValue("FC.rotation_speed");
    const double bkgChopper1Speed =
        runDetails.getPropertyAsSingleValue("BC1.rotation_speed");
    const double bkgChopper2Speed =
        runDetails.getPropertyAsSingleValue("BC2.rotation_speed");

    if (std::abs(bkgChopper1Speed - bkgChopper2Speed) > 1) {
      throw std::invalid_argument(
          "Background choppers 1 and 2 have different speeds");
    }

    n_pulses = fermiChopperSpeed / bkgChopper1Speed / 4;
  } else if (m_instrumentName == "IN6") {
    fermiChopperSpeed =
        runDetails.getPropertyAsSingleValue("Fermi.rotation_speed");
    const double suppressorSpeed =
        runDetails.getPropertyAsSingleValue("Suppressor.rotation_speed");

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
 * @param monitors List of monitor data
 */
void LoadILLTOF2::loadDataIntoTheWorkSpace(
    NeXus::NXEntry &entry, const std::vector<std::vector<int>> &monitors) {

  g_log.debug() << "Loading data into the workspace...\n";
  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();

  // Put tof in an array
  auto &X0 = m_localWorkspace->mutableX(0);
  for (size_t i = 0; i < m_numberOfChannels + 1; ++i) {
    X0[i] = m_timeOfFlightDelay + m_channelWidth * static_cast<double>(i) -
            m_channelWidth / 2; // to make sure the bin centre is correct
  }

  // The binning for monitors is considered the same as for detectors
  size_t spec = 0;

  auto const &instrument = m_localWorkspace->getInstrument();

  const std::vector<detid_t> detectorIDs = instrument->getDetectorIDs(true);

  Progress progress(this, 0.0, 1.0, m_numberOfTubes * m_numberOfPixelsPerTube);

  loadSpectra(spec, m_numberOfTubes, detectorIDs, data, progress);

  g_log.debug() << "Loading detector data into the workspace: DONE!\n";

  /**
   * IN4 Rosace detectors are in a different NeXus entry
   */
  if (m_instrumentName == "IN4") {
    g_log.debug() << "Loading data into the workspace: IN4 Rosace!\n";
    // read in the data
    NXData dataGroupRosace =
        entry.openNXData("instrument/Detector_Rosace/data");
    NXInt dataRosace = dataGroupRosace.openIntData();
    auto numberOfTubes = static_cast<size_t>(dataRosace.dim0());
    // load the counts from the file into memory
    dataRosace.load();

    Progress progressRosace(this, 0.0, 1.0,
                            numberOfTubes * m_numberOfPixelsPerTube);

    loadSpectra(spec, numberOfTubes, detectorIDs, dataRosace, progressRosace);
  }

  const auto monitorIDs = instrument->getMonitors();

  for (size_t i = 0; i < monitors.size(); ++i) {
    const auto &monitor = monitors[i];
    m_localWorkspace->setHistogram(spec, m_localWorkspace->binEdges(0),
                                   Counts(monitor.begin(), monitor.end()));
    m_localWorkspace->getSpectrum(spec).setDetectorID(monitorIDs[i]);
    spec++;
  }
}

/**
 * Loops over all the pixels and loads the correct spectra. Called for each set
 * of detector types in the workspace.
 *
 * @param spec The current spectrum id
 * @param numberOfTubes The number of detector tubes in the workspace
 * @param detectorIDs A list of all of the detector IDs
 * @param data The NeXus data to load into the workspace
 * @param progress The progress monitor
 */
void LoadILLTOF2::loadSpectra(size_t &spec, const size_t numberOfTubes,
                              const std::vector<detid_t> &detectorIDs,
                              const NXInt &data, Progress &progress) {
  for (size_t i = 0; i < numberOfTubes; ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      m_localWorkspace->setHistogram(
          spec, m_localWorkspace->binEdges(0),
          Counts(data_p, data_p + m_numberOfChannels));
      m_localWorkspace->getSpectrum(spec).setDetectorID(detectorIDs[spec]);
      spec++;
      progress.report();
    }
  }
}

/**
 * Runs LoadInstrument to attach the instrument to the workspace
 */
void LoadILLTOF2::runLoadInstrument() {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  loadInst->setPropertyValue("InstrumentName", m_instrumentName);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  loadInst->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(false));
  loadInst->execute();
}

} // namespace DataHandling
} // namespace Mantid
