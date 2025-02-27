// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMLZ.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;
using HistogramData::BinEdges;
using HistogramData::Counts;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMLZ)

/** Constructor
 */
LoadMLZ::LoadMLZ()
    : m_numberOfTubes{0}, m_numberOfPixelsPerTube{0}, m_numberOfChannels{0}, m_numberOfHistograms{0},
      m_monitorElasticPeakPosition{0}, m_wavelength{0.0}, m_channelWidth{0.0}, m_timeOfFlightDelay{0.0},
      m_monitorCounts{0}, m_chopper_speed{0.0}, m_chopper_ratio{0}, m_l1{0.0}, m_l2{0.0}, m_t1{0.0},
      m_supportedInstruments{"TOFTOF", "DNS"} {}

/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadMLZ::name() const { return "LoadMLZ"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadMLZ::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadMLZ::category() const { return "DataHandling\\Nexus"; }

/** Initialize the algorithm's properties.
 */
void LoadMLZ::init() {
  const std::vector<std::string> exts{".nxs", ".hdf", ".hd5"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "File path of the Data file to load");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
}

/** Execute the algorithm.
 */
void LoadMLZ::exec() {
  // Retrieve filename
  std::string filenameData = getPropertyValue("Filename");

  // open the root node
  NeXus::NXRoot dataRoot(filenameData);
  NXEntry dataFirstEntry = dataRoot.openFirstEntry();

  loadInstrumentDetails(dataFirstEntry);
  loadTimeDetails(dataFirstEntry);

  initWorkspace(dataFirstEntry);

  // load the instrument from the IDF
  runLoadInstrument();
  initInstrumentSpecific();

  loadDataIntoTheWorkSpace(dataFirstEntry);
  loadRunDetails(dataFirstEntry); // must run after runLoadInstrument
  loadExperimentDetails(dataFirstEntry);
  maskDetectors(dataFirstEntry);

  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Return the confidence with which this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMLZ::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  // fields existent only at the MLZ
  if (descriptor.isEntry("/Scan/wavelength") && descriptor.isEntry("/Scan/title") && descriptor.isEntry("/Scan/mode")) {
    return 80;
  } else {
    return 0;
  }
}

/**
 * Loads Masked detectors from the /Scan/instrument/Detector/pixel_mask
 */
void LoadMLZ::maskDetectors(const NeXus::NXEntry &entry) {
  // path to the pixel_mask
  std::string pmpath = "instrument/detector/pixel_mask";

  NeXus::NXInt pmdata = entry.openNXInt(pmpath);
  // load the counts from the file into memory
  pmdata.load();
  g_log.debug() << "PMdata size: " << pmdata.size() << '\n';
  std::vector<int> masked_detectors(pmdata(), pmdata() + pmdata.size());

  g_log.debug() << "Number of masked detectors: " << masked_detectors.size() << '\n';

  auto &detInfo = m_localWorkspace->mutableDetectorInfo();
  std::vector<size_t> indicesToMask;
  for (auto masked_detector : masked_detectors) {
    g_log.debug() << "List of masked detectors: ";
    g_log.debug() << masked_detector;
    g_log.debug() << ", ";
    try {
      indicesToMask.emplace_back(detInfo.indexOf(masked_detector));
    } catch (std::out_of_range &) {
      g_log.warning() << "Invalid detector ID " << masked_detector << ". Found while running LoadMLZ\n";
    }
  }
  g_log.debug() << '\n';

  for (const auto index : indicesToMask)
    detInfo.setMasked(index, true);
}

/**
 * Set the instrument name along with its path on the nexus file
 */
void LoadMLZ::loadInstrumentDetails(const NeXus::NXEntry &firstEntry) {

  m_instrumentPath = LoadHelper::findInstrumentNexusPath(firstEntry);

  if (m_instrumentPath.empty()) {
    throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
  }

  m_instrumentName = LoadHelper::getStringFromNexusPath(firstEntry, m_instrumentPath + "/name");

  if (std::find(m_supportedInstruments.begin(), m_supportedInstruments.end(), m_instrumentName) ==
      m_supportedInstruments.end()) {
    std::string message = "The instrument " + m_instrumentName + " is not valid for this loader!";
    throw std::runtime_error(message);
  }

  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param entry :: The Nexus entry
 *
 */
void LoadMLZ::initWorkspace(const NeXus::NXEntry &entry) //, const std::vector<std::vector<int> >&monitors)
{
  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  m_numberOfChannels = static_cast<size_t>(data.dim2());
  m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;

  g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << '\n';
  g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << '\n';
  g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << '\n';

  // Now create the output workspace
  m_localWorkspace = WorkspaceFactory::Instance().create("Workspace2D", m_numberOfHistograms, m_numberOfChannels + 1,
                                                         m_numberOfChannels);
  m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Function to do specific instrument stuff
 *
 */
void LoadMLZ::initInstrumentSpecific() {
  // Read data from IDF: distance source-sample and distance sample-detectors
  m_l1 = m_localWorkspace->spectrumInfo().l1();
  m_l2 = m_localWorkspace->spectrumInfo().l2(1);

  g_log.debug() << "L1: " << m_l1 << ", L2: " << m_l2 << '\n';
}

/**
 * Load the time details from the nexus file.
 * @param entry :: The Nexus entry
 */
void LoadMLZ::loadTimeDetails(const NeXus::NXEntry &entry) {

  m_wavelength = entry.getFloat("wavelength");

  // Monitor can be monitor or Monitor
  std::string monitorName;
  if (entry.containsGroup("monitor"))
    monitorName = "monitor";
  else if (entry.containsGroup("Monitor"))
    monitorName = "Monitor";
  else {
    std::string message("Cannot find monitor/Monitor in the Nexus file!");
    g_log.error(message);
    throw std::runtime_error(message);
  }

  m_monitorCounts = entry.getInt(monitorName + "/integral");

  m_monitorElasticPeakPosition = entry.getInt(monitorName + "/elastic_peak");

  NXFloat time_of_flight_data = entry.openNXFloat(monitorName + "/time_of_flight");
  time_of_flight_data.load();

  // The entry "monitor/time_of_flight", has 3 fields:
  // channel width [microseconds], number of channels, Time of flight delay
  m_channelWidth = time_of_flight_data[0] * 50.e-3;
  m_timeOfFlightDelay = time_of_flight_data[2] * 50.e-3;

  g_log.debug("Nexus Data:");
  g_log.debug() << " MonitorCounts: " << m_monitorCounts << '\n';
  g_log.debug() << " ChannelWidth (microseconds): " << m_channelWidth << '\n';
  g_log.debug() << " Wavelength (angstroems): " << m_wavelength << '\n';
  g_log.debug() << " ElasticPeakPosition: " << m_monitorElasticPeakPosition << '\n';
  g_log.debug() << " TimeOfFlightDelay (microseconds): " << m_timeOfFlightDelay << '\n';

  m_chopper_speed = entry.getFloat("instrument/chopper/rotation_speed");

  m_chopper_ratio = entry.getInt("instrument/chopper/ratio");

  g_log.debug() << " ChopperSpeed: " << m_chopper_speed << '\n';
  g_log.debug() << " ChopperRatio: " << m_chopper_ratio << '\n';
}

/**
 * Load information about the run.
 * People from ISIS have this...
 * TODO: They also have a lot of info in XML format!
 *
 * @param entry :: The Nexus entry
 */
void LoadMLZ::loadRunDetails(NXEntry &entry) {

  API::Run &runDetails = m_localWorkspace->mutableRun();

  std::string runNum = entry.getString("entry_identifier"); // run_number");
  std::string run_num = boost::lexical_cast<std::string>(runNum);
  runDetails.addProperty("run_number", run_num);

  std::string start_time = entry.getString("start_time");
  runDetails.addProperty("run_start", start_time);

  std::string end_time = entry.getString("end_time");
  runDetails.addProperty("run_end", end_time);

  runDetails.addProperty("wavelength", m_wavelength, "Angstrom", true);

  double ei = LoadHelper::calculateEnergy(m_wavelength);
  runDetails.addProperty<double>("Ei", ei, "meV", true); // overwrite

  int duration = entry.getInt("duration");
  runDetails.addProperty("duration", duration, "Seconds", true);

  std::string mode = entry.getString("mode");
  runDetails.addProperty("mode", mode);

  std::string title = entry.getString("title");
  m_localWorkspace->setTitle(title);

  // Check if temperature is defined
  NXClass sample = entry.openNXGroup("sample");
  if (sample.containsDataSet("temperature")) {
    double temperature = entry.getFloat("sample/temperature");
    runDetails.addProperty("temperature", temperature, "K", true);
  }

  runDetails.addProperty("monitor_counts", static_cast<double>(m_monitorCounts));
  runDetails.addProperty("chopper_speed", m_chopper_speed);
  runDetails.addProperty("chopper_ratio", m_chopper_ratio);
  runDetails.addProperty("channel_width", m_channelWidth, "microseconds", true);

  // Calculate number of full time channels - use to crop workspace - S. Busch's
  // method
  double full_channels =
      floor(30. * m_chopper_ratio / (m_chopper_speed) * 1.e6 / m_channelWidth); // channelWidth in microsec.
  runDetails.addProperty("full_channels", full_channels);

  // Proposal title
  std::string proposal_title = entry.getString("proposal");
  runDetails.addProperty("proposal_title", proposal_title);

  // proposal number
  std::string proposal_number = entry.getString("proposal_number");
  runDetails.addProperty("proposal_number", proposal_number);

  // users
  std::string user_name = entry.getString("user2/name");
  runDetails.addProperty("experiment_team", user_name);

  runDetails.addProperty("EPP", m_monitorElasticPeakPosition);
  runDetails.addProperty("TOF1", m_t1, "microseconds", true);

  // set instrument parameter Efixed, catch error, but don't stop
  try {
    auto setPar = createChildAlgorithm("SetInstrumentParameter");
    setPar->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    setPar->setProperty("ParameterName", "Efixed");
    setPar->setProperty("ParameterType", "Number");
    setPar->setProperty("Value", std::to_string(ei));
    setPar->execute();
  } catch (...) {
    g_log.warning("Cannot set the instrument parameter Efixed.");
  }
}

/**
 * Load data about the Experiment.
 *
 * TODO: This is very incomplete.
 *
 * @param entry :: The Nexus entry
 */
void LoadMLZ::loadExperimentDetails(const NXEntry &entry) {
  // TODO: Do the rest
  // Pick out the geometry information

  std::string description = boost::lexical_cast<std::string>(entry.getFloat("sample/description"));

  m_localWorkspace->mutableSample().setName(description);
}

/**
 * Loads all the spectra into the workspace, including that from the monitor
 *
 * @param entry :: The Nexus entry
 */
void LoadMLZ::loadDataIntoTheWorkSpace(const NeXus::NXEntry &entry) {
  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  data.load();

  m_t1 = LoadHelper::calculateTOF(m_l1, m_wavelength) * 1.0e+6;
  g_log.debug() << " t1 (microseconds): " << m_t1 << '\n';

  std::vector<double> detectorTofBins(m_numberOfChannels + 1);
  for (size_t i = 0; i < m_numberOfChannels + 1; ++i) {
    detectorTofBins[i] = m_channelWidth * static_cast<double>(static_cast<int>(i)) + m_t1 + m_channelWidth / 2;
  }

  // Assign calculated bins to first X axis
  BinEdges edges(std::move(detectorTofBins));

  Progress progress(this, 0.0, 1.0, m_numberOfTubes * m_numberOfPixelsPerTube);
  size_t spec = 0;
  for (size_t i = 0; i < m_numberOfTubes; ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
      // Assign Y
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);

      m_localWorkspace->setHistogram(spec, edges, Counts(data_p, data_p + m_numberOfChannels));

      ++spec;
      progress.report();
    }
  }
}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadMLZ::runLoadInstrument() {
  auto loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    g_log.debug() << "InstrumentName" << m_instrumentName << '\n';
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loadInst->execute();
  } catch (...) {
    g_log.warning("Cannot load the instrument definition.");
  }
}

} // namespace Mantid::DataHandling
