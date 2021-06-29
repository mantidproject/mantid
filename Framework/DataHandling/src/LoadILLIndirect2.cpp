// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLIndirect2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cmath>
#include <nexus/napi.h>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLIndirect2)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLIndirect2::name() const { return "LoadILLIndirect"; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLIndirect2::category() const { return "DataHandling\\Nexus;ILL\\Indirect"; }

//----------------------------------------------------------------------------------------------

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLIndirect2::confidence(Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the ILL
  if (descriptor.pathExists("/entry0/wavelength")               // ILL
      && descriptor.pathExists("/entry0/experiment_identifier") // ILL
      && descriptor.pathExists("/entry0/mode")                  // ILL
      && ((descriptor.pathExists("/entry0/instrument/Doppler/mirror_sense") &&
           descriptor.pathExists("/entry0/dataSD/SingleD_data")) // IN16B new
          || (descriptor.pathExists("/entry0/instrument/Doppler/doppler_frequency") &&
              descriptor.pathExists("/entry0/dataSD/dataSD")) // IN16B old
          )) {
    return 80;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLIndirect2::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "File path of the Data file to load");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  std::vector<std::string> loadingOptions{"Spectrometer", "Diffractometer"};
  declareProperty("LoadDetectors", "Spectrometer", std::make_shared<StringListValidator>(loadingOptions),
                  "Select the type of data to load from IN16B.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLIndirect2::exec() {

  // Retrieve filename
  const std::string filenameData = getPropertyValue("Filename");

  m_loadOption = getPropertyValue("LoadDetectors");

  size_t progressSteps = m_loadOption == "Diffractometer" ? 5 : 7;
  Progress progress(this, 0., 1., progressSteps);

  // open the root node
  NeXus::NXRoot dataRoot(filenameData);
  NXEntry firstEntry = dataRoot.openFirstEntry();

  // Load Data details (number of tubes, channels, mode, etc)
  loadDataDetails(firstEntry);
  progress.report("Loaded metadata");

  const std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
  setInstrumentName(firstEntry, instrumentPath);

  initWorkSpace();
  progress.report("Initialised the workspace");

  loadNexusEntriesIntoProperties(filenameData);
  progress.report("Loaded data details");

  if (m_loadOption == "Diffractometer") {
    loadDiffractionData(firstEntry);
  } else {
    loadDataIntoTheWorkSpace(firstEntry);
  }
  progress.report("Loaded the data");

  runLoadInstrument();
  progress.report("Loaded the instrument");

  if (m_loadOption == "Spectrometer") {
    moveSingleDetectors(firstEntry);
    progress.report("Loaded the single detectors");

    rotateTubes();
    progress.report("Rotating tubes if necessary");
  }
  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
}

/**
 * Set member variable with the instrument name
 * @param firstEntry : nexus entry
 * @param instrumentNamePath : nexus path to instrument name
 */
void LoadILLIndirect2::setInstrumentName(const NeXus::NXEntry &firstEntry, const std::string &instrumentNamePath) {
  if (instrumentNamePath.empty()) {
    std::string message("Cannot set the instrument name from the Nexus file!");
    g_log.error(message);
    throw std::runtime_error(message);
  }
  m_instrumentName = m_loader.getStringFromNexusPath(firstEntry, instrumentNamePath + "/name");
  boost::to_upper(m_instrumentName); // "IN16b" in file, keep it upper case.
  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';
}

std::string LoadILLIndirect2::getDataPath(NeXus::NXEntry &entry) {
  NeXus::NXClass instrument = entry.openNXGroup("instrument");
  if (m_loadOption == "Diffractometer") {

    if (instrument.containsGroup("DiffDet")) {
      return "instrument/DiffDet/data";
    } else if (entry.containsGroup("dataDiffDet")) {
      return "dataDiffDet/DiffDet_data";
    } else {
      throw std::runtime_error("Cannot find diffraction detector data in the Nexus file. Make sure "
                               "they exist or load the spectrometer data instead.");
    }
  } else {
    return "data";
  }
}

/**
 * Load Data details (number of tubes, channels, etc)
 * @param entry First entry of nexus file
 */
void LoadILLIndirect2::loadDataDetails(NeXus::NXEntry &entry) {

  // find the data
  std::string dataPath = getDataPath(entry);

  // read in the data
  NXData dataGroup = entry.openNXData(dataPath);
  NXInt data = dataGroup.openIntData();

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
  m_numberOfChannels = static_cast<size_t>(data.dim2());

  try {
    NXInt mode = entry.openNXInt("acquisition_mode");
    mode.load();
    m_bats = mode[0] == 1;
  } catch (...) {
    g_log.information() << "Unable to read acquisition_mode, assuming doppler";
  }

  // check which single detectors are enabled, and store their indices
  if (m_loadOption == "Spectrometer") {
    NXData dataSDGroup = entry.openNXData("dataSD");
    NXInt dataSD = dataSDGroup.openIntData();

    for (int i = 1; i <= dataSD.dim0(); ++i) {
      try {
        std::string entryNameFlagSD = boost::str(boost::format("instrument/SingleD/tubes%i_function") % i);
        NXFloat flagSD = entry.openNXFloat(entryNameFlagSD);
        flagSD.load();

        if (flagSD[0] == 1.0) // is enabled
        {
          m_activeSDIndices.insert(i);
        }
      } catch (...) {
        // if the flags are not present in the file (e.g. old format), load all
        m_activeSDIndices.insert(i);
      }
    }
    m_numberOfSimpleDetectors = m_activeSDIndices.size();
    g_log.information() << "Number of activated single detectors is: " << m_numberOfSimpleDetectors << std::endl;

    try {
      NXFloat firstTubeAngle = entry.openNXFloat("instrument/PSD/PSD angle 1");
      firstTubeAngle.load();
      m_firstTubeAngleRounded = static_cast<size_t>(std::round(10 * firstTubeAngle[0]));
    } catch (...) {
      g_log.information() << "Unable to read first tube anlge, assuming 25.1";
    }
  } else {
    m_numberOfSimpleDetectors = 0;
  }
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 */
void LoadILLIndirect2::initWorkSpace() {
  const size_t nHistograms = m_numberOfTubes * m_numberOfPixelsPerTube + m_numberOfMonitors + m_numberOfSimpleDetectors;
  m_localWorkspace =
      WorkspaceFactory::Instance().create("Workspace2D", nHistograms, m_numberOfChannels + 1, m_numberOfChannels);
  const auto timeChannels =
      make_cow<HistogramData::HistogramX>(m_numberOfChannels + 1, HistogramData::LinearGenerator(0.0, 1.0));
  for (size_t i = 0; i < nHistograms; ++i) {
    m_localWorkspace->setSharedX(i, timeChannels);
  }
  m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Empty");
  m_localWorkspace->setYUnitLabel("Counts");
}

/**
 * Load data found in nexus file in general, indirect mode.
 * @param entry :: The Nexus entry
 */
void LoadILLIndirect2::loadDataIntoTheWorkSpace(NeXus::NXEntry &entry) {

  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  data.load();

  NXData dataSDGroup = entry.openNXData("dataSD");
  NXInt dataSD = dataSDGroup.openIntData();
  dataSD.load();

  NXData dataMonGroup = entry.openNXData("monitor/data");
  NXInt dataMon = dataMonGroup.openIntData();
  dataMon.load();

  // First, Monitor
  // Assign Y
  int *monitor_p = &dataMon(0, 0);
  m_localWorkspace->dataY(0).assign(monitor_p, monitor_p + m_numberOfChannels);
  // Assign Error
  MantidVec &E = m_localWorkspace->dataE(0);
  std::transform(monitor_p, monitor_p + m_numberOfChannels, E.begin(), [](const double v) { return std::sqrt(v); });
  // Then Tubes
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_localWorkspace))
  for (int i = 0; i < static_cast<int>(m_numberOfTubes); ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
      const size_t index = i * m_numberOfPixelsPerTube + j + m_numberOfMonitors;
      // Assign Y
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      m_localWorkspace->dataY(index).assign(data_p, data_p + m_numberOfChannels);
      // Assign Error
      MantidVec &E = m_localWorkspace->dataE(index);
      std::transform(data_p, data_p + m_numberOfChannels, E.begin(), [](const double v) { return std::sqrt(v); });
    }
  }
  // Then add Simple Detector (SD)
  size_t offset = m_numberOfTubes * m_numberOfPixelsPerTube + m_numberOfMonitors;
  for (auto &index : m_activeSDIndices) {
    // Assign Y, note that index starts from 1
    int *dataSD_p = &dataSD(index - 1, 0, 0);
    m_localWorkspace->dataY(offset).assign(dataSD_p, dataSD_p + m_numberOfChannels);
    // Assign Error
    MantidVec &E = m_localWorkspace->dataE(offset);
    std::transform(dataSD_p, dataSD_p + m_numberOfChannels, E.begin(), [](const double v) { return std::sqrt(v); });
    ++offset;
  }
}

/**
 * @brief LoadILLIndirect2::loadDiffractionData
 * Load IN16B diffraction data from the Nexus file when requested.
 * @param entry
 */
void LoadILLIndirect2::loadDiffractionData(NeXus::NXEntry &entry) {
  NeXus::NXClass instrument = entry.openNXGroup("instrument");

  // first, determine version
  bool newVersion = instrument.containsDataSet("version");

  // find the path to the data
  std::string dataPath = getDataPath(entry);

  NXData dataGroup = entry.openNXData(dataPath);

  NXInt data = dataGroup.openIntData();
  data.load();

  NXData dataMonGroup = entry.openNXData("monitor/data");
  NXInt dataMon = dataMonGroup.openIntData();
  dataMon.load();

  // First, Monitor
  // Assign Y
  int *monitor_p = &dataMon(0, 0);
  m_localWorkspace->dataY(0).assign(monitor_p, monitor_p + m_numberOfChannels);
  // Assign Error
  MantidVec &E = m_localWorkspace->dataE(0);
  std::transform(monitor_p, monitor_p + m_numberOfChannels, E.begin(), [](const double v) { return std::sqrt(v); });

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_localWorkspace))
  for (int i = 0; i < static_cast<int>(m_numberOfTubes); ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
      size_t index;
      if (!newVersion) {
        // Then Tubes
        if (i == 2 || i == 3) {
          index = (m_numberOfTubes - 1 - i) * m_numberOfPixelsPerTube + (m_numberOfPixelsPerTube - 1 - j) +
                  m_numberOfMonitors;
        } else {
          index = (m_numberOfTubes - 1 - i) * m_numberOfPixelsPerTube + j + m_numberOfMonitors;
        }
      } else {
        index = i * m_numberOfPixelsPerTube + j + m_numberOfMonitors;
      }
      // Assign Y
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      m_localWorkspace->dataY(index).assign(data_p, data_p + m_numberOfChannels);
      // Assign Error
      MantidVec &E = m_localWorkspace->dataE(index);
      std::transform(data_p, data_p + m_numberOfChannels, E.begin(), [](const double v) { return std::sqrt(v); });
    }
  }
}

/**
 * @brief Loads the sample logs
 * @param nexusfilename
 */
void LoadILLIndirect2::loadNexusEntriesIntoProperties(const std::string &nexusfilename) {

  API::Run &runDetails = m_localWorkspace->mutableRun();
  NXhandle nxfileID;
  NXstatus stat = NXopen(nexusfilename.c_str(), NXACC_READ, &nxfileID);
  if (stat == NX_ERROR) {
    g_log.debug() << "convertNexusToProperties: Error loading " << nexusfilename;
    throw Kernel::Exception::FileError("Unable to open File:", nexusfilename);
  }
  m_loader.addNexusFieldsToWsRun(nxfileID, runDetails);
  runDetails.addProperty("Facility", std::string("ILL"));
  NXclose(&nxfileID);
}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadILLIndirect2::runLoadInstrument() {
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("Filename", getInstrumentFilePath());
  loadInst->setPropertyValue("InstrumentName", m_instrumentName);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
  loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  loadInst->execute();
}

/**
 * Makes up the full path of the relevant IDF dependent on first tube angle and
 * mode
 * @return : the full path to the corresponding IDF
 */
std::string LoadILLIndirect2::getInstrumentFilePath() {
  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  std::string idf = m_instrumentName;
  if (m_loadOption == "Diffractometer") {
    idf += "D";
  } else if (!m_bats && m_firstTubeAngleRounded == 251) {
    // load the instrument with the first tube analyser focused at the midpoint
    // of sample to tube center
    idf += "F";
  }
  Poco::Path file(idf + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

/**
 * @brief Moves the component to the given 2theta
 * @param componentName
 * @param twoTheta
 */
void LoadILLIndirect2::moveComponent(const std::string &componentName, double twoTheta) {
  Geometry::Instrument_const_sptr instrument = m_localWorkspace->getInstrument();
  Geometry::IComponent_const_sptr component = instrument->getComponentByName(componentName);
  double r, theta, phi;
  V3D oldPos = component->getPos();
  oldPos.getSpherical(r, theta, phi);
  V3D newPos;
  newPos.spherical(r, twoTheta, phi);
  g_log.debug() << componentName << " : t = " << theta << " ==> t = " << twoTheta << "\n";
  auto &compInfo = m_localWorkspace->mutableComponentInfo();
  const auto componentIndex = compInfo.indexOf(component->getComponentID());
  compInfo.setPosition(componentIndex, newPos);
}

/**
 * IN16B has a few single detectors that are place around the sample.
 * They are moved according to some values in the nexus file.
 * @param entry : the nexus entry
 */
void LoadILLIndirect2::moveSingleDetectors(NeXus::NXEntry &entry) {
  std::string prefix("single_tube_");
  int index = 1;
  for (auto i : m_activeSDIndices) {
    std::string angleEntry = boost::str(boost::format("instrument/SingleD/SD%i angle") % i);
    NXFloat angleSD = entry.openNXFloat(angleEntry);
    angleSD.load();
    g_log.debug("Moving single detector " + std::to_string(i) + " to t=" + std::to_string(angleSD[0]));
    moveComponent(prefix + std::to_string(index), angleSD[0]);
    index++;
  }
}

/**
 * The detector has two positions. By IDF the first tube is at 25.1 Degree as
 * opening angle from Z- (Z+ is the beam direction). But it could be also 33.1,
 * in which case all the tubes should be rotated around the sample.
 */
void LoadILLIndirect2::rotateTubes() {
  if (m_firstTubeAngleRounded != 251 && m_firstTubeAngleRounded != 331) {
    g_log.warning() << "Unexpected first tube angle found: " << m_firstTubeAngleRounded
                    << " degrees. Check your instrument configuration. "
                       "Assuming 25.1 degrees instead.";
  } else if (m_firstTubeAngleRounded == 331) {
    auto rotator = this->createChildAlgorithm("RotateInstrumentComponent");
    rotator->setProperty("Workspace", m_localWorkspace);
    rotator->setProperty("RelativeRotation", false);
    rotator->setPropertyValue("ComponentName", "psds");
    rotator->setProperty("Y", 1.);
    rotator->setProperty("Angle", -8.);
    rotator->execute();
  }
}

} // namespace DataHandling
} // namespace Mantid
