#include "MantidDataHandling/LoadILLIndirect.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string.hpp>

#include <nexus/napi.h>
#include <numeric>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLIndirect)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLIndirect::LoadILLIndirect()
    : API::IFileLoader<Kernel::NexusDescriptor>(), m_numberOfTubes(0),
      m_numberOfPixelsPerTube(0), m_numberOfChannels(0),
      m_numberOfSimpleDetectors(0), m_numberOfHistograms(0) {
  m_supportedInstruments.emplace_back("IN16B");
  useAlgorithm("LoadILLIndirect", 2);
  deprecatedDate("01.04.2017");
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLIndirect::name() const { return "LoadILLIndirect"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLIndirect::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLIndirect::category() const {
  return "DataHandling\\Nexus";
}

//----------------------------------------------------------------------------------------------

/**
* Return the confidence with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadILLIndirect::confidence(Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the ILL
  if (descriptor.pathExists("/entry0/wavelength")               // ILL
      && descriptor.pathExists("/entry0/experiment_identifier") // ILL
      && descriptor.pathExists("/entry0/mode")                  // ILL
      &&
      ((descriptor.pathExists("/entry0/instrument/Doppler/mirror_sense") &&
        descriptor.pathExists("/entry0/dataSD/SingleD_data")) // IN16B new
       ||
       (descriptor.pathExists("/entry0/instrument/Doppler/doppler_frequency") &&
        descriptor.pathExists("/entry0/dataSD/dataSD")) // IN16B old
       )) {
    return 70;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLIndirect::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "File path of the Data file to load");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLIndirect::exec() {
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

  initWorkSpace(firstEntry, monitorsData);

  g_log.debug("Building properties...");
  loadNexusEntriesIntoProperties(filenameData);

  g_log.debug("Loading data...");
  loadDataIntoTheWorkSpace(firstEntry, monitorsData);

  // load the instrument from the IDF if it exists
  g_log.debug("Loading instrument definition...");
  runLoadInstrument();

  // moveSingleDetectors(); Work in progress

  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
* Set member variable with the instrument name
*/
void LoadILLIndirect::setInstrumentName(const NeXus::NXEntry &firstEntry,
                                        const std::string &instrumentNamePath) {

  if (instrumentNamePath == "") {
    std::string message("Cannot set the instrument name from the Nexus file!");
    g_log.error(message);
    throw std::runtime_error(message);
  }
  m_instrumentName =
      m_loader.getStringFromNexusPath(firstEntry, instrumentNamePath + "/name");
  boost::to_upper(m_instrumentName); // "IN16b" in file, keep it upper case.
  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
}

/**
* Load Data details (number of tubes, channels, etc)
* @param entry First entry of nexus file
*/
void LoadILLIndirect::loadDataDetails(NeXus::NXEntry &entry) {
  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  m_numberOfChannels = static_cast<size_t>(data.dim2());

  NXData dataSDGroup = entry.openNXData("dataSD");
  NXInt dataSD = dataSDGroup.openIntData();

  m_numberOfSimpleDetectors = static_cast<size_t>(dataSD.dim0());
}

/**
   * Load monitors data found in nexus file
   *
   * @param entry :: The Nexus entry
   *
   */
std::vector<std::vector<int>>
LoadILLIndirect::loadMonitors(NeXus::NXEntry &entry) {
  // read in the data
  g_log.debug("Fetching monitor data...");

  NXData dataGroup = entry.openNXData("monitor/data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();

  // For the moment, we are aware of only one monitor entry, but we keep the
  // generalized case of n monitors

  std::vector<std::vector<int>> monitors(1);
  std::vector<int> monitor(data(), data() + data.size());
  monitors[0].swap(monitor);
  return monitors;
}

/**
   * Creates the workspace and initialises member variables with
   * the corresponding values
   *
   * @param entry :: The Nexus entry
   * @param monitorsData :: Monitors data already loaded
   *
   */
void LoadILLIndirect::initWorkSpace(
    NeXus::NXEntry & /*entry*/, std::vector<std::vector<int>> monitorsData) {

  // dim0 * m_numberOfPixelsPerTube is the total number of detectors
  m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;

  g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << '\n';
  g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << '\n';
  g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << '\n';
  g_log.debug() << "NumberOfSimpleDetectors: " << m_numberOfSimpleDetectors
                << '\n';
  g_log.debug() << "Monitors: " << monitorsData.size() << '\n';
  g_log.debug() << "Monitors[0]: " << monitorsData[0].size() << '\n';

  // Now create the output workspace

  m_localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D",
      m_numberOfHistograms + monitorsData.size() + m_numberOfSimpleDetectors,
      m_numberOfChannels + 1, m_numberOfChannels);

  m_localWorkspace->getAxis(0)->unit() =
      UnitFactory::Instance().create("Empty");

  m_localWorkspace->setYUnitLabel("Counts");
}

/**
   * Load data found in nexus file
   *
   * @param entry :: The Nexus entry
   * @param monitorsData :: Monitors data already loaded
   *
   */
void LoadILLIndirect::loadDataIntoTheWorkSpace(
    NeXus::NXEntry &entry, std::vector<std::vector<int>> monitorsData) {

  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();

  // Same for Simple Detectors
  NXData dataSDGroup = entry.openNXData("dataSD");
  NXInt dataSD = dataSDGroup.openIntData();
  // load the counts from the file into memory
  dataSD.load();

  size_t spec = 0;
  size_t nb_monitors = monitorsData.size();
  size_t nb_SD_detectors = dataSD.dim0();

  Progress progress(this, 0.0, 1.0, m_numberOfTubes * m_numberOfPixelsPerTube +
                                        nb_monitors + nb_SD_detectors);

  // Assign fake values to first X axis
  const HistogramData::BinEdges histoBinEdges(
      m_numberOfChannels + 1, HistogramData::LinearGenerator(1.0, 1.0));

  // First, Monitor
  for (size_t im = 0; im < nb_monitors; im++) {

    int *monitor_p = monitorsData[im].data();
    const HistogramData::Counts histoCounts(monitor_p,
                                            monitor_p + m_numberOfChannels);
    m_localWorkspace->setHistogram(im, histoBinEdges, std::move(histoCounts));

    progress.report();
  }

  // Then Tubes
  for (size_t i = 0; i < m_numberOfTubes; ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {

      // Assign Y
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      const HistogramData::Counts histoCounts(data_p,
                                              data_p + m_numberOfChannels);
      m_localWorkspace->setHistogram((spec + nb_monitors), histoBinEdges,
                                     std::move(histoCounts));

      ++spec;
      progress.report();
    }
  } // for m_numberOfTubes

  // Then add Simple Detector (SD)
  for (int i = 0; i < dataSD.dim0(); ++i) {

    // Assign Y
    int *dataSD_p = &dataSD(i, 0, 0);
    const HistogramData::Counts histoCounts(dataSD_p,
                                            dataSD_p + m_numberOfChannels);
    const HistogramData::CountStandardDeviations histoErrors(m_numberOfChannels,
                                                             0.0);

    m_localWorkspace->setHistogram((spec + nb_monitors + i), histoBinEdges,
                                   std::move(histoCounts),
                                   std::move(histoErrors));
    progress.report();
  }

} // LoadILLIndirect::loadDataIntoTheWorkSpace

void LoadILLIndirect::loadNexusEntriesIntoProperties(
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
   * Run the Child Algorithm LoadInstrument.
   */
void LoadILLIndirect::runLoadInstrument() {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(true));
    loadInst->execute();

  } catch (...) {
    g_log.information("Cannot load the instrument definition.");
  }
}

void LoadILLIndirect::moveComponent(const std::string &componentName,
                                    double twoTheta, double offSet) {

  try {

    Geometry::Instrument_const_sptr instrument =
        m_localWorkspace->getInstrument();
    Geometry::IComponent_const_sptr component =
        instrument->getComponentByName(componentName);

    double r, theta, phi, newTheta, newR;
    V3D oldPos = component->getPos();
    oldPos.getSpherical(r, theta, phi);

    newTheta = twoTheta;
    newR = offSet;

    V3D newPos;
    newPos.spherical(newR, newTheta, phi);

    m_localWorkspace->mutableDetectorInfo().setPosition(*component, newPos);

  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    throw std::runtime_error("Error when trying to move the " + componentName +
                             " : NotFoundError");
  } catch (std::runtime_error &) {
    throw std::runtime_error("Error when trying to move the " + componentName +
                             " : runtime_error");
  }
}

/**
 * IN16B has a few single detectors that are place around the sample.
 * They are moved according to some values in the nexus file.
 * This is not implemented yet.
 */
void LoadILLIndirect::moveSingleDetectors() {

  std::string prefix("single_tube_");

  for (int i = 1; i <= 8; i++) {
    std::string componentName = prefix + std::to_string(i);

    moveComponent(componentName, i * 20.0, 2.0 + i / 10.0);
  }
}

} // namespace DataHandling
} // namespace Mantid
