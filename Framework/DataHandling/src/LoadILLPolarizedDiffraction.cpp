// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLPolarizedDiffraction.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Geometry;
using namespace Kernel;
using namespace NeXus;
using Types::Core::DateAndTime;

namespace {
// This defines the number of detector banks in D7
constexpr size_t D7_NUMBER_BANKS = 3;
// This defines the number of physical pixels in D7
constexpr size_t D7_NUMBER_PIXELS = 132;
// This defines the number of pixels per bank in D7
constexpr size_t D7_NUMBER_PIXELS_BANK = 44;
// This defines the number of monitors in the instrument. If there are cases
// where this is no longer one this decleration should be moved.
constexpr size_t NUMBER_MONITORS = 2;
// This defines Time Of Flight measurement mode switch value
constexpr size_t TOF_MODE_ON = 1;
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLPolarizedDiffraction)

/// Returns confidence. @see IFileLoader::confidence
int LoadILLPolarizedDiffraction::confidence(NexusDescriptor &descriptor) const {

  // fields existent only at the ILL Diffraction
  if (descriptor.pathExists("/entry0/D7")) {
    return 80;
  } else {
    return 0;
  }
}

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLPolarizedDiffraction::name() const { return "LoadILLPolarizedDiffraction"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLPolarizedDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLPolarizedDiffraction::category() const { return "DataHandling\\Nexus;ILL\\Diffraction"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLPolarizedDiffraction::summary() const {
  return "Loads ILL D7 instrument polarized diffraction nexus files.";
}

/**
 * Constructor
 */
LoadILLPolarizedDiffraction::LoadILLPolarizedDiffraction() : IFileLoader<NexusDescriptor>() {}

/**
 * Initialize the algorithm's properties.
 */
void LoadILLPolarizedDiffraction::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "File path of the data file to load");
  declareProperty(std::make_unique<WorkspaceProperty<API::WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  const std::vector<std::string> positionCalibrationOptions{"None", "Nexus", "YIGFile"};
  declareProperty("PositionCalibration", "None", std::make_shared<StringListValidator>(positionCalibrationOptions),
                  "Select the type of pixel position calibration. If None, the pixel "
                  "positions are read from IDF file. If Nexus, the positions are read from "
                  "Nexus file. If YIGFile, then the calibration twotheta data is loaded "
                  "from a user-defined calibration file.");

  declareProperty(std::make_unique<FileProperty>("YIGFilename", "", FileProperty::OptionalLoad, ".xml"),
                  "File path of the YIG calibration data file to load.");
  setPropertySettings("YIGFilename",
                      std::make_unique<Kernel::EnabledWhenProperty>("PositionCalibration", IS_EQUAL_TO, "YIGFile"));
  declareProperty("ConvertToScatteringAngle", false, "Convert the bin edges to scattering angle.", Direction::Input);
  declareProperty("TransposeMonochromatic", false, "Transpose the 2D workspace with monochromatic data",
                  Direction::Input);
  const std::vector<std::string> TOFUnitOptions{"UncalibratedTime", "TimeChannels"};
  declareProperty("TOFUnits", TOFUnitOptions[0], std::make_shared<StringListValidator>(TOFUnitOptions),
                  "The choice of X-axis units for Time-Of-Flight data.");
}

std::map<std::string, std::string> LoadILLPolarizedDiffraction::validateInputs() {
  std::map<std::string, std::string> issues;
  if (getPropertyValue("PositionCalibration") == "YIGFile" && getPropertyValue("YIGFilename") == "") {
    issues["PositionCalibration"] = "YIG-based position calibration of detectors requested but "
                                    "the file was not provided.";
  }
  return issues;
}

/**
 * Executes the algorithm.
 */
void LoadILLPolarizedDiffraction::exec() {

  Progress progress(this, 0, 1, 2);

  m_fileName = getPropertyValue("Filename");
  m_outputWorkspaceGroup = std::make_shared<API::WorkspaceGroup>();
  m_wavelength = 0;

  progress.report("Loading the detector polarization analysis data");
  loadData();

  progress.report("Loading the metadata");
  loadMetaData();

  setProperty("OutputWorkspace", m_outputWorkspaceGroup);
}

/**
 * Loads the polarized detector data, sets up workspaces and labels
 *  according to the measurement type and data dimensions
 */
void LoadILLPolarizedDiffraction::loadData() {

  // open the root entry
  NXRoot dataRoot(m_fileName);

  // read each entry
  for (auto entryNumber = 0; entryNumber < static_cast<int>((dataRoot.groups().size())); entryNumber++) {
    NXEntry entry = dataRoot.openEntry("entry" + std::to_string(entryNumber));
    m_instName = entry.getString("D7/name");

    std::string start_time = entry.getString("start_time");
    start_time = m_loadHelper.dateTimeInIsoFormat(start_time);

    // init the workspace with proper number of histograms and number of
    // channels
    auto workspace = initStaticWorkspace(entry);

    // load the instrument
    loadInstrument(workspace, start_time);

    // rotate detectors to their position during measurement
    moveTwoTheta(entry, workspace);

    // prepare axes for data
    std::vector<double> axis = prepareAxes(entry);

    // load data from file
    std::string dataName = "data/Detector_data";
    NXUInt data = entry.openNXDataSet<unsigned int>(dataName);
    data.load();

    // Assign detector counts
    PARALLEL_FOR_IF(Kernel::threadSafe(*workspace))
    for (auto pixel_no = 0; pixel_no < static_cast<int>(D7_NUMBER_PIXELS); ++pixel_no) {
      auto &spectrum = workspace->mutableY(pixel_no);
      auto &errors = workspace->mutableE(pixel_no);
      for (auto channel_no = 0; channel_no < static_cast<int>(m_numberOfChannels); ++channel_no) {
        unsigned int counts = data(pixel_no, 0, channel_no);
        spectrum[channel_no] = counts;
        errors[channel_no] = std::sqrt(counts);
      }
      workspace->mutableX(pixel_no) = axis;
    }

    // load and assign monitor data
    for (auto monitor_no = static_cast<int>(D7_NUMBER_PIXELS);
         monitor_no < static_cast<int>(D7_NUMBER_PIXELS + NUMBER_MONITORS); ++monitor_no) {
      NXUInt monitorData = entry.openNXDataSet<unsigned int>(
          "monitor" + std::to_string(monitor_no + 1 - static_cast<int>(D7_NUMBER_PIXELS)) + "/data");
      monitorData.load();
      auto &spectrum = workspace->mutableY(monitor_no);
      auto &errors = workspace->mutableE(monitor_no);
      for (auto channel_no = 0; channel_no < static_cast<int>(m_numberOfChannels); channel_no++) {
        unsigned int counts = monitorData(0, 0, channel_no);
        spectrum[channel_no] = counts;
        errors[channel_no] = std::sqrt(counts);
      }
      workspace->mutableX(monitor_no) = axis;
    }

    // convert the spectrum axis to scattering angle
    if (getProperty("ConvertToScatteringAngle")) {
      workspace = convertSpectrumAxis(workspace);
    }
    // transpose monochromatic data distribution
    if (getProperty("TransposeMonochromatic") && m_acquisitionMode != TOF_MODE_ON) {
      workspace = transposeMonochromatic(workspace);
    }

    // adds the current entry workspace to the output group
    m_outputWorkspaceGroup->addWorkspace(workspace);
    entry.close();
  }
  dataRoot.close();
}

/**
 * Dumps the metadata from the file for each entry separately
 */
void LoadILLPolarizedDiffraction::loadMetaData() {

  // Open NeXus file
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(m_fileName.c_str(), NXACC_READ, &nxHandle);

  if (nxStat != NX_ERROR) {
    for (auto workspaceId = 0; workspaceId < m_outputWorkspaceGroup->getNumberOfEntries(); ++workspaceId) {
      MatrixWorkspace_sptr workspace =
          std::static_pointer_cast<API::MatrixWorkspace>(m_outputWorkspaceGroup->getItem(workspaceId));
      auto const entryName = std::string("entry" + std::to_string(workspaceId));
      m_loadHelper.addNexusFieldsToWsRun(nxHandle, workspace->mutableRun(), entryName);
      if (m_wavelength != 0) {
        workspace->mutableRun().addProperty("monochromator.wavelength", m_wavelength, true);
      }
    }
    NXclose(&nxHandle);
  }
}

/**
 * Initializes the output workspace based on the resolved instrument.
 * If there are multiple entries in the file and the current entry
 * is not the first one, the returned workspace is a clone
 * of the workspace from the first entry
 * @param entry : entry linked with the returned workspace
 * @return : workspace with the correct data dimensions
 */
API::MatrixWorkspace_sptr LoadILLPolarizedDiffraction::initStaticWorkspace(const NXEntry &entry) {
  const size_t nSpectra = D7_NUMBER_PIXELS + NUMBER_MONITORS;

  // Set number of channels
  NXInt acquisitionMode = entry.openNXInt("acquisition_mode");
  acquisitionMode.load();
  m_acquisitionMode = acquisitionMode[0];
  if (m_acquisitionMode == TOF_MODE_ON) {
    NXFloat timeOfFlightInfo = entry.openNXFloat("D7/Detector/time_of_flight");
    timeOfFlightInfo.load();
    m_numberOfChannels = size_t(timeOfFlightInfo[1]);
  } else {
    m_numberOfChannels = 1;
  }

  API::MatrixWorkspace_sptr workspace =
      WorkspaceFactory::Instance().create("Workspace2D", nSpectra, m_numberOfChannels + 1, m_numberOfChannels);

  // Set x axis units
  if (m_acquisitionMode == TOF_MODE_ON) {
    if (getPropertyValue("TOFUnits") == "TimeChannels") {
      auto lblUnit = std::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
      lblUnit->setLabel("Time channel", Units::Symbol::EmptyLabel);
      workspace->getAxis(0)->unit() = lblUnit;
    } else {
      workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    }
  } else {
    workspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  }
  // Set y axis unit
  workspace->setYUnit("Counts");

  // check the polarization direction and set the workspace title
  std::string polDirection = entry.getString("D7/POL/actual_state");
  std::string flipperState = entry.getString("D7/POL/actual_stateB1B2");
  workspace->setTitle(polDirection.substr(0, 1) + "_" + flipperState);
  return workspace;
}
/**
 * Runs LoadInstrument as child to link the instrument to workspace
 * @param workspace : workspace with data from the first entry
 * @param startTime :: the date the run started, in ISO compliant format
 */
void LoadILLPolarizedDiffraction::loadInstrument(API::MatrixWorkspace_sptr workspace, const std::string &startTime) {

  // the start time is needed in the workspace when loading the parameter file
  workspace->mutableRun().addProperty("start_time", startTime);

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("Filename", m_instName + "_Definition.xml");
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", workspace);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->execute();
}

/**
 * Loads 2theta for each detector pixel from either the nexus file or the
 * Instrument Parameter File
 * @param workspace : workspace with loaded instrument
 * @param entry : entry from which the pixel 2theta positions will be read
 * @param bankId : bank ID for which 2theta positions will be read
 * @return : vector of pixel 2theta positions in the chosen bank
 */
std::vector<double> LoadILLPolarizedDiffraction::loadTwoThetaDetectors(const API::MatrixWorkspace_sptr workspace,
                                                                       const NXEntry &entry, const int bankId) {

  std::vector<double> twoTheta(static_cast<int>(D7_NUMBER_PIXELS_BANK));

  if (getPropertyValue("PositionCalibration") == "Nexus") {
    NXFloat twoThetaPixels = entry.openNXFloat("D7/Detector/bank" + std::to_string(bankId) + "_offset");
    twoThetaPixels.load();
    float *twoThetaDataStart = twoThetaPixels();
    float *twoThetaDataEnd = twoThetaDataStart + D7_NUMBER_PIXELS_BANK;
    twoTheta.assign(twoThetaDataStart, twoThetaDataEnd);
  } else {
    IAlgorithm_sptr loadIpf = createChildAlgorithm("LoadParameterFile");
    loadIpf->setPropertyValue("Filename", getPropertyValue("YIGFilename"));
    loadIpf->setProperty("Workspace", workspace);
    loadIpf->execute();

    Instrument_const_sptr instrument = workspace->getInstrument();
    IComponent_const_sptr currentBank = instrument->getComponentByName(std::string("bank" + std::to_string(bankId)));

    m_wavelength = currentBank->getNumberParameter("wavelength")[0];

    for (auto pixel_no = 0; pixel_no < static_cast<int>(D7_NUMBER_PIXELS_BANK); pixel_no++) {
      twoTheta[pixel_no] = currentBank->getNumberParameter("twoTheta_pixel_" + std::to_string(pixel_no + 1))[0];
    }
  }
  return twoTheta;
}

/**
 * Loads offsets and slopes for each detector bank from the workspace entry
 * @param workspace : workspace with loaded instrument
 * @param bankId : bank ID of the relevant bank
 * @return : vector of the bank slope and offset
 */
std::vector<double> LoadILLPolarizedDiffraction::loadBankParameters(const API::MatrixWorkspace_sptr workspace,
                                                                    const int bankId) {
  std::vector<double> bankParameters;

  Instrument_const_sptr instrument = workspace->getInstrument();
  IComponent_const_sptr currentBank = instrument->getComponentByName(std::string("bank" + std::to_string(bankId)));

  auto slope = currentBank->getNumberParameter("gradient")[0];
  bankParameters.push_back(slope);
  auto offset = currentBank->getNumberParameter("offset")[0];
  bankParameters.push_back(offset);

  return bankParameters;
}

/**
 * Rotates each pixel to its corresponding 2theta read from the file
 * @param entry : entry from which the 2theta positions will be read
 * @param workspace : workspace containing the instrument being moved
 */
void LoadILLPolarizedDiffraction::moveTwoTheta(const NXEntry &entry, API::MatrixWorkspace_sptr workspace) {

  Instrument_const_sptr instrument = workspace->getInstrument();

  auto &componentInfo = workspace->mutableComponentInfo();
  for (auto bank_no = 0; bank_no < static_cast<int>(D7_NUMBER_BANKS); ++bank_no) {
    NXFloat twoThetaBank =
        entry.openNXFloat("D7/2theta/actual_bank" + std::to_string(bank_no + 2)); // detector bank IDs start at 2
    twoThetaBank.load();
    if (getPropertyValue("PositionCalibration") == "None") {
      Quat rotation(-twoThetaBank[0], V3D(0, 1, 0));
      IComponent_const_sptr currentBank =
          instrument->getComponentByName(std::string("bank" + std::to_string(bank_no + 2)));
      const auto componentIndex = componentInfo.indexOf(currentBank->getComponentID());
      componentInfo.setRotation(componentIndex, rotation);
    } else {
      std::vector<double> twoThetaPixels = loadTwoThetaDetectors(workspace, entry, bank_no + 2);
      std::vector<double> bankParameters{1, 0}; // slope, offset
      if (getPropertyValue("PositionCalibration") == "YIGFile") {
        bankParameters = loadBankParameters(workspace, bank_no + 2);
      }
      for (auto pixel_no = 0; pixel_no < static_cast<int>(D7_NUMBER_PIXELS_BANK); ++pixel_no) {
        auto const pixelIndex = bank_no * static_cast<int>(D7_NUMBER_PIXELS_BANK) + pixel_no;
        auto const pixel = componentInfo.componentID(pixelIndex);
        V3D position = pixel->getPos();
        double radius, theta, phi;
        position.getSpherical(radius, theta, phi);
        position.spherical(radius, bankParameters[0] * twoThetaBank[0] - bankParameters[1] - twoThetaPixels[pixel_no],
                           phi);
        componentInfo.setPosition(pixelIndex, position);
      }
    }
  }
}

/**
 * Prepares values for bin edges depending of measurement type
 * @param entry : entry from which the number of channels and measurement type
 * will be read
 * @return : returns vector with bin edges
 */
std::vector<double> LoadILLPolarizedDiffraction::prepareAxes(const NXEntry &entry) {
  // check the mode of measurement and prepare axes for data
  std::vector<double> axes;

  if (m_acquisitionMode == TOF_MODE_ON) {
    NXFloat timeOfFlightInfo = entry.openNXFloat("D7/Detector/time_of_flight");
    timeOfFlightInfo.load();
    auto channelWidth = static_cast<double>(timeOfFlightInfo[0]);
    m_numberOfChannels = size_t(timeOfFlightInfo[1]);
    auto tofDelay = timeOfFlightInfo[2];
    for (auto channel_no = 0; channel_no <= static_cast<int>(m_numberOfChannels); channel_no++) {
      if (getPropertyValue("TOFUnits") == "UncalibratedTime") {
        axes.push_back(tofDelay + channel_no * channelWidth);
      } else {
        axes.push_back(channel_no);
      }
    }
  } else {
    double wavelength = 0;
    if (m_wavelength != 0) {
      wavelength = m_wavelength;
    } else {
      NXFloat wavelengthNexus = entry.openNXFloat("D7/monochromator/wavelength");
      wavelengthNexus.load();
      wavelength = wavelengthNexus[0];
    }
    axes.push_back(static_cast<double>(wavelength * 0.99));
    axes.push_back(static_cast<double>(wavelength * 1.01));
  }
  return axes;
}

/**
 * Converts the spectrum axis to scattering angle
 * @param workspace : workspace to change the
 */
API::MatrixWorkspace_sptr LoadILLPolarizedDiffraction::convertSpectrumAxis(API::MatrixWorkspace_sptr workspace) {
  IAlgorithm_sptr convertSpectrumAxis = createChildAlgorithm("ConvertSpectrumAxis");
  convertSpectrumAxis->initialize();
  convertSpectrumAxis->setProperty("InputWorkspace", workspace);
  convertSpectrumAxis->setProperty("OutputWorkspace", "__unused_for_child");
  convertSpectrumAxis->setProperty("Target", "SignedTheta");
  convertSpectrumAxis->setProperty("EMode", "Direct");
  convertSpectrumAxis->setProperty("OrderAxis", false);
  convertSpectrumAxis->execute();
  workspace = convertSpectrumAxis->getProperty("OutputWorkspace");

  IAlgorithm_sptr changeSign = createChildAlgorithm("ConvertAxisByFormula");
  changeSign->initialize();
  changeSign->setProperty("InputWorkspace", workspace);
  changeSign->setProperty("OutputWorkspace", "__unused_for_child");
  changeSign->setProperty("Axis", "Y");
  changeSign->setProperty("Formula", "-y");
  changeSign->execute();
  return changeSign->getProperty("OutputWorkspace");
}

/**
 * Transposes given 2D workspace with monochromatic data
 * @param workspace : workspace to be transposed
 */
API::MatrixWorkspace_sptr LoadILLPolarizedDiffraction::transposeMonochromatic(API::MatrixWorkspace_sptr workspace) {
  IAlgorithm_sptr transpose = createChildAlgorithm("Transpose");
  transpose->initialize();
  transpose->setProperty("InputWorkspace", workspace);
  transpose->setProperty("OutputWorkspace", "__unused_for_child");
  transpose->execute();
  return transpose->getProperty("OutputWorkspace");
}

} // namespace DataHandling
} // namespace Mantid
