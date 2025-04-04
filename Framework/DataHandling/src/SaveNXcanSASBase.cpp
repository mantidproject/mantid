// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNXcanSASBase.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataHandling/NXcanSASHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/LambdaValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;

namespace {

/// Property names.
namespace StandardProperties {
static const std::string INPUT_WORKSPACE = "InputWorkspace";
static const std::string FILENAME = "Filename";
static const std::string RADIATION_SOURCE = "RadiationSource";
static const std::string DETECTOR_NAMES = "DetectorNames";
static const std::string TRANSMISSION = "Transmission";
static const std::string TRANSMISSION_CAN = "TransmissionCan";
static const std::string SAMPLE_TRANS_RUN_NUMBER = "SampleTransmissionRunNumber";
static const std::string SAMPLE_DIRECT_RUN_NUMBER = "SampleDirectRunNumber";
static const std::string CAN_SCATTER_RUN_NUMBER = "CanScatterRunNumber";
static const std::string CAN_DIRECT_RUN_NUMBER = "CanDirectRunNumber";
static const std::string BKG_SUB_WORKSPACE = "BackgroundSubtractionWorkspace";
static const std::string BKG_SUB_SCALE = "BackgroundSubtractionScaleFactor";
static const std::string GEOMETRY = "Geometry";
static const std::string SAMPLE_HEIGHT = "SampleHeight";
static const std::string SAMPLE_WIDTH = "SampleWidth";
static const std::string SAMPLE_THICKNESS = "SampleThickness";
} // namespace StandardProperties

namespace PolProperties {
static const std::string INPUT_SPIN_STATES = "InputSpinStates";
static const std::string POLARIZER_COMP_NAME = "PolarizerComponentName";
static const std::string ANALYZER_COMP_NAME = "AnalyzerComponentName";
static const std::string FLIPPER_COMP_NAMES = "FlipperComponentNames";
static const std::string MAG_FIELD_STRENGTH_LOGNAME = "MagneticFieldStrengthLogName";
static const std::string MAG_FIELD_DIR = "MagneticFieldDirection";
std::map<std::string, std::string> POL_COMPONENTS = {
    {"polarizer", POLARIZER_COMP_NAME}, {"analyzer", ANALYZER_COMP_NAME}, {"flipper", FLIPPER_COMP_NAMES}};

} // namespace PolProperties

bool hasUnit(const std::string &unitToCompareWith, const MatrixWorkspace_sptr &ws) {
  if (ws->axes() == 0) {
    return false;
  }
  auto const unit = ws->getAxis(0)->unit();
  return (unit && unit->unitID() == unitToCompareWith);
}

void areAxesNumeric(const MatrixWorkspace_sptr &workspace, int numberOfDims = 1) {
  std::vector<int> indices(numberOfDims);
  std::generate(indices.begin(), indices.end(), [i = 0]() mutable { return i++; });
  if (numberOfDims == 0 || !std::all_of(indices.cbegin(), indices.cend(), [workspace](auto const &index) {
        return workspace->getAxis(index)->isNumeric();
      })) {
    throw std::invalid_argument("Incorrect number of numerical axis");
  }
}

bool checkValidMatrixWorkspace(const Workspace_sptr &ws) {
  auto const &ws_input = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  return (ws_input && hasUnit("MomentumTransfer", ws_input) && ws_input->isCommonBins());
}

std::string validateGroupWithProperties(const Workspace_sptr &ws) {
  if (!ws) {
    return "Workspace has to be a valid workspace";
  }

  if (ws->isGroup()) {
    auto const &groupItems = std::dynamic_pointer_cast<WorkspaceGroup>(ws)->getAllItems();
    if (std::any_of(groupItems.cbegin(), groupItems.cend(),
                    [](auto const &childWs) { return !checkValidMatrixWorkspace(childWs); })) {
      return "Workspace must have common bins and Momentum transfer units";
    }
    return "";
  }

  if (!checkValidMatrixWorkspace(ws)) {
    return "Workspace must have common bins and Momentum transfer units";
  }
  return "";
}

} // namespace

namespace Mantid::DataHandling {

void SaveNXcanSASBase::initStandardProperties() {
  // Standard NXcanSAS properties
  auto groupValidator = std::make_shared<Kernel::LambdaValidator<Workspace_sptr>>(validateGroupWithProperties);
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(StandardProperties::INPUT_WORKSPACE, "",
                                                                 Kernel::Direction::Input, groupValidator),
                  "The input workspace, which must be in units of Q. Can be a 1D or a 2D workspace.");
  declareProperty(std::make_unique<FileProperty>(StandardProperties::FILENAME, "", API::FileProperty::Save, ".h5"),
                  "The name of the .h5 file to save");

  std::vector<std::string> radiationSourceOptions{"Spallation Neutron Source",
                                                  "Pulsed Reactor Neutron Source",
                                                  "Reactor Neutron Source",
                                                  "Synchrotron X-ray Source",
                                                  "Pulsed Muon Source",
                                                  "Rotating Anode X-ray",
                                                  "Fixed Tube X-ray",
                                                  "neutron",
                                                  "x-ray",
                                                  "muon",
                                                  "electron"};
  declareProperty(StandardProperties::RADIATION_SOURCE, "Spallation Neutron Source",
                  std::make_shared<Kernel::StringListValidator>(radiationSourceOptions), "The type of radiation used.");
  declareProperty(StandardProperties::DETECTOR_NAMES, "",
                  "Specify in a comma separated list, which detectors to store "
                  "information about; \nwhere each name must match a name "
                  "given for a detector in the [[IDF|instrument definition "
                  "file (IDF)]]. \nIDFs are located in the instrument "
                  "sub-directory of the Mantid install directory.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      StandardProperties::TRANSMISSION, "", Kernel::Direction::Input, PropertyMode::Optional,
                      std::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
                  "The transmission workspace. Optional. If given, will be saved at "
                  "TransmissionSpectrum");

  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      StandardProperties::TRANSMISSION_CAN, "", Kernel::Direction::Input, PropertyMode::Optional,
                      std::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
                  "The transmission workspace of the Can. Optional. If given, will be "
                  "saved at TransmissionSpectrum");

  declareProperty(StandardProperties::SAMPLE_TRANS_RUN_NUMBER, "",
                  "The run number for the sample transmission workspace. Optional.");
  declareProperty(StandardProperties::SAMPLE_DIRECT_RUN_NUMBER, "",
                  "The run number for the sample direct workspace. Optional.");
  declareProperty(StandardProperties::CAN_SCATTER_RUN_NUMBER, "",
                  "The run number for the can scatter workspace. Optional.");
  declareProperty(StandardProperties::CAN_DIRECT_RUN_NUMBER, "",
                  "The run number for the can direct workspace. Optional.");

  declareProperty(
      StandardProperties::BKG_SUB_WORKSPACE, "",
      "The name of the workspace used in the scaled background subtraction, to be included in the metadata. Optional.");
  declareProperty(
      StandardProperties::BKG_SUB_SCALE, 0.0,
      "The scale factor used in the scaled background subtraction, to be included in the metadata. Optional.");

  std::vector<std::string> const geometryOptions{"Cylinder", "FlatPlate", "Flat plate", "Disc", "Unknown"};
  declareProperty(StandardProperties::GEOMETRY, "Unknown",
                  std::make_shared<Kernel::StringListValidator>(geometryOptions),
                  "The geometry type of the collimation.");
  declareProperty(StandardProperties::SAMPLE_HEIGHT, 0.0,
                  "The height of the collimation element in mm. If specified as 0 it will not be recorded.");
  declareProperty(StandardProperties::SAMPLE_WIDTH, 0.0,
                  "The width of the collimation element in mm. If specified as 0 it will not be recorded.");
  declareProperty(StandardProperties::SAMPLE_THICKNESS, 0.0,
                  "The thickness of the sample in mm. If specified as 0 it will not be recorded.");
}

void SaveNXcanSASBase::initPolarizedProperties() {
  const auto spinStateValidator = std::make_shared<Algorithms::SpinStateValidator>(
      std::unordered_set<int>{2, 4}, false, SpinStateNXcanSAS::SPIN_PARA, SpinStateNXcanSAS::SPIN_ANTIPARA, true,
      SpinStateNXcanSAS::SPIN_ZERO);

  declareProperty(PolProperties::INPUT_SPIN_STATES, "", spinStateValidator,
                  "The order of the spin states in the input group workspace: +1 Polarization parallel to polarizer, "
                  "-1 antiparallel and 0 no polarization");
  declareProperty(PolProperties::POLARIZER_COMP_NAME, "",
                  "The name of the Polarizer Component as defined in the IDF. i.e. 'short-polarizer'");
  declareProperty(PolProperties::ANALYZER_COMP_NAME, "",
                  "The name of the Analyzer Component as defined in the IDF. i.e. 'helium-analyzer'");
  declareProperty(PolProperties::FLIPPER_COMP_NAMES, "",
                  "Comma separated list of flipper components as defined in the IDF i.e. 'RF-flipper");
  declareProperty(PolProperties::MAG_FIELD_STRENGTH_LOGNAME, "",
                  "The name of  sample logs in which the magnetic field strength is stored");
  declareProperty(PolProperties::MAG_FIELD_DIR, "",
                  "Direction of the magnetic field on the sample: comma separated vector"
                  "with three values: Polar, Azimuthal and Rotation angles");
}

std::map<std::string, std::string> SaveNXcanSASBase::validateStandardInputs() const {
  std::map<std::string, std::string> result;

  /* If input workspace is a group, check that each group members is a valid 2D Workspace,
     otherwise check that the input is a valid 2D workspace.*/
  Workspace_sptr const workspace = getProperty(StandardProperties::INPUT_WORKSPACE);
  auto valid2DWorkspace = [](const Workspace_sptr &ws) {
    return ws && std::dynamic_pointer_cast<const Mantid::DataObjects::Workspace2D>(ws);
  };
  if (workspace->isGroup()) {
    auto const &groupItems = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)->getAllItems();
    if (std::any_of(groupItems.cbegin(), groupItems.cend(),
                    [&](auto const &childWs) { return !valid2DWorkspace(childWs); })) {
      result.emplace("InputWorkspace",
                     "All input workspaces in the input group must be a Workspace2D wit numeric axis.");
    }
  } else {
    if (!valid2DWorkspace(workspace)) {
      result.emplace("InputWorkspace", "The InputWorkspace must be a Workspace2D with numeric axis.");
    }
  }

  // Transmission data should be 1D
  MatrixWorkspace_sptr transmission = getProperty(StandardProperties::TRANSMISSION);
  MatrixWorkspace_sptr transmissionCan = getProperty(StandardProperties::TRANSMISSION_CAN);

  auto checkTransmission = [&result](const MatrixWorkspace_sptr &trans, const std::string &propertyName) {
    if (trans->getNumberHistograms() != 1) {
      result.emplace(propertyName, "The input workspaces for transmissions have to be 1D.");
    }
  };

  if (transmission) {
    checkTransmission(transmission, StandardProperties::TRANSMISSION);
  }
  if (transmissionCan) {
    checkTransmission(transmissionCan, StandardProperties::TRANSMISSION_CAN);
  }

  return result;
}

std::map<std::string, std::string> SaveNXcanSASBase::validatePolarizedInputs() const {
  std::map<std::string, std::string> result;

  Workspace_sptr const workspace = getProperty(StandardProperties::INPUT_WORKSPACE);
  std::string const spins = getProperty(PolProperties::INPUT_SPIN_STATES);
  auto const spinVec = VectorHelper::splitStringIntoVector<std::string>(spins);

  if (!workspace->isGroup())
    result.emplace(StandardProperties::INPUT_WORKSPACE,
                   "Input Workspaces for polarized data can only be workspace groups.");
  else {
    auto const wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    auto const &entries = wsGroup->getNumberOfEntries();
    if (entries != 2 && entries != 4) {
      result.emplace(StandardProperties::INPUT_WORKSPACE,
                     "Input Group Workspace can only contain 2 or 4 workspace members.");
    }

    if (entries != static_cast<int>(spinVec.size())) {
      result.emplace(PolProperties::INPUT_SPIN_STATES, "The number of spin states is different than the number of"
                                                       " member workspaces on the InputWorkspace group");
    }

    auto const wsVec = wsGroup->getAllItems();
    // The group has been validated in StandardInputs, so workspaces are safely downcasted to MatrixWorkspaces
    auto dim0 = getWorkspaceDimensionality(std::dynamic_pointer_cast<MatrixWorkspace>(wsVec.at(0)));
    if (std::any_of(wsVec.begin(), wsVec.end(), [&dim0](const Workspace_sptr &ws) {
          return dim0 != getWorkspaceDimensionality(std::dynamic_pointer_cast<MatrixWorkspace>(ws));
        })) {
      result.emplace(StandardProperties::INPUT_WORKSPACE, "All workspaces in group must have the same dimensionality");
    }
  }

  // Validating spin strings
  if (spinVec.size() == 4 && std::any_of(spinVec.begin(), spinVec.end(), [](std::string const &spinPair) {
        return (spinPair.find(SpinStateNXcanSAS::SPIN_ZERO) != std::string::npos);
      })) {
    result.emplace(PolProperties::INPUT_SPIN_STATES, "Full polarized group can't contain spin state 0");
  }

  if (spinVec.size() == 2) {
    if (std::any_of(spinVec.begin(), spinVec.end(),
                    [](const std::string &state) { return state.find('1') == std::string::npos; })) {
      result.emplace(PolProperties::INPUT_SPIN_STATES, "There can't be 00 state");
    }
    auto noPin =
        spinVec[0].starts_with(SpinStateNXcanSAS::SPIN_ZERO) && spinVec[1].starts_with(SpinStateNXcanSAS::SPIN_ZERO);
    auto noPout =
        spinVec[0].ends_with(SpinStateNXcanSAS::SPIN_ZERO) && spinVec[1].ends_with(SpinStateNXcanSAS::SPIN_ZERO);
    if (noPin == noPout) {
      result.emplace(PolProperties::INPUT_SPIN_STATES,
                     "The 0 polarized state can only be either Pin or Pout for 2 spin configurations");
    }
  }

  std::string const magneticFieldDirection = getProperty(PolProperties::MAG_FIELD_DIR);
  if (!magneticFieldDirection.empty()) {
    auto direction = VectorHelper::splitStringIntoVector<std::string>(magneticFieldDirection);
    try {
      std::for_each(direction.begin(), direction.end(), [](const std::string &val) { std::stod(val); });
    } catch (const std::invalid_argument &) {
      result.emplace(PolProperties::MAG_FIELD_DIR, "Some value of the magnetic field direction vector is not a number");
    }
    if (direction.size() != 3) {
      result.emplace(PolProperties::MAG_FIELD_DIR,
                     "Magnetic Field Direction should contain 3 comma separated values to represent a 3D vector");
    }
  }

  return result;
}

/**
 * Adds standard metadata to a NXcanSAS file format.
 * 1. Adds instrument metata: Detectors, source and aperture.
 * 2. Adds sample metadata
 * 3. Adds process metadata: Run number, version info.
 * 4. If there's transmission or transmission can. Adds process entry for the
 * workspaces and transmission/transmission can data in a new group.
 * 5. If there's information for background subtraction workspace, adds new entry
 * with the scale and workspace name.
 * @param workspace: Workspace to add instrument from
 * @param sasEntry: sas group in which to store metadata
 */
void SaveNXcanSASBase::addStandardMetadata(const MatrixWorkspace_sptr &workspace, H5::Group &sasEntry) const {
  const auto &radiationSource = getPropertyValue(StandardProperties::RADIATION_SOURCE);
  const std::string &geometry = getProperty(StandardProperties::GEOMETRY);
  double beamHeight = getProperty(StandardProperties::SAMPLE_HEIGHT);
  double beamWidth = getProperty(StandardProperties::SAMPLE_WIDTH);
  double sampleThickness = getProperty(StandardProperties::SAMPLE_THICKNESS);
  const auto &detectorNames = getPropertyValue(StandardProperties::DETECTOR_NAMES);

  MatrixWorkspace_sptr transmissionSample = getProperty(StandardProperties::TRANSMISSION);
  MatrixWorkspace_sptr transmissionCan = getProperty(StandardProperties::TRANSMISSION_CAN);

  // Add the instrument information
  const auto detectors = Kernel::VectorHelper::splitStringIntoVector<std::string>(detectorNames);
  addInstrument(sasEntry, workspace, radiationSource, geometry, beamHeight, beamWidth, detectors);

  // Add the sample information
  addSample(sasEntry, sampleThickness);

  // Get additional run numbers
  const auto &sampleTransmissionRun = getPropertyValue(StandardProperties::SAMPLE_TRANS_RUN_NUMBER);
  const auto &sampleDirectRun = getPropertyValue(StandardProperties::SAMPLE_DIRECT_RUN_NUMBER);
  const auto &canScatterRun = getPropertyValue(StandardProperties::CAN_SCATTER_RUN_NUMBER);
  const auto &canDirectRun = getPropertyValue(StandardProperties::CAN_DIRECT_RUN_NUMBER);

  // Get scaled background subtraction information
  const auto &scaledBgSubWorkspace = getPropertyValue(StandardProperties::BKG_SUB_WORKSPACE);

  addProcess(sasEntry, workspace, transmissionCan);

  // Add additional process information
  auto process = sasEntry.openGroup(sasProcessGroupName);

  if (transmissionCan) {
    H5Util::write(process, sasProcessTermCanScatter, canScatterRun);
    H5Util::write(process, sasProcessTermCanDirect, canDirectRun);
  }
  if (transmissionSample) {
    H5Util::write(process, sasProcessTermSampleDirect, sampleDirectRun);
    H5Util::write(process, sasProcessTermSampleTrans, sampleTransmissionRun);
  }

  if (!scaledBgSubWorkspace.empty()) {
    double scaledBgSubScaleFactor = getProperty(StandardProperties::BKG_SUB_SCALE);
    H5Util::write(process, sasProcessTermScaledBgSubWorkspace, scaledBgSubWorkspace);
    H5Util::writeScalarDataSetWithStrAttributes(process, sasProcessTermScaledBgSubScaleFactor, scaledBgSubScaleFactor,
                                                {});
  }

  // Add the transmissions for sample
  if (transmissionSample) {
    addTransmission(sasEntry, transmissionSample, sasTransmissionSpectrumNameSampleAttrValue);
  }

  // Add the transmissions for can
  if (transmissionCan) {
    addTransmission(sasEntry, transmissionCan, sasTransmissionSpectrumNameCanAttrValue);
  }
}

/**
 * Adds polarized metadata to a NXcanSAS file format.
 * 1. Adds polarizer components defined in properties per name: Polarizers, Analyzers, Flippers.
 * 2. Adds magnetic field strength data taken from sample logs, property input is the name of the sample log
 * in which the magnetic field strength is stored.
 * 3. Adds magnetic field direction in spherical coordinates with input taken from properties.
 * @param workspace: First workspace of polarized input workspace group.
 * @param sasEntry: sas group in which to store metadata.
 */
void SaveNXcanSASBase::addPolarizedMetadata(const MatrixWorkspace_sptr &workspace, H5::Group &sasEntry) const {

  for (auto const &[compType, compVec] : createPolarizedComponentMap()) {
    for (size_t i = 0; i < compVec.size(); i++) {
      auto const suffix = compVec.size() > 1 ? addDigit(i + 1) : "";
      addPolarizer(sasEntry, workspace, compVec.at(i), compType, suffix);
    }
  }
  std::string const &sasSampleMagneticField = getProperty(PolProperties::MAG_FIELD_STRENGTH_LOGNAME);
  std::string const &magneticFieldDirStr = getProperty(PolProperties::MAG_FIELD_DIR);
  addSampleEMFields(sasEntry, workspace, sasSampleMagneticField, magneticFieldDirStr);
}

/**
 * Creates a component map to access the polarizer component names defined in input properties on the corresponding IDF.
 */
std::map<std::string, std::vector<std::string>> SaveNXcanSASBase::createPolarizedComponentMap() const {
  // Assumption here is that we will pass the IDF component names as comma separated lists
  std::map<std::string, std::vector<std::string>> componentMap;
  for (auto const &[compType, compName] : PolProperties::POL_COMPONENTS) {
    std::string const &componentName = getProperty(compName);
    if (!componentName.empty()) {
      auto const componentVec = VectorHelper::splitStringIntoVector<std::string>(componentName);
      componentMap.emplace(std::make_pair(compType, componentVec));
    }
  }
  return componentMap;
}

/**
 * Add the sasEntry to the sasroot.
 * @param file: Handle to the NXcanSAS file
 * @param workspace: the workspace to store
 * @param suffix: suffix for sas entry group name. Default is "01"
 * @return sasEntry group object
 */
H5::Group SaveNXcanSASBase::addSasEntry(H5::H5File &file, const MatrixWorkspace_sptr &workspace,
                                        const std::string &suffix) const {
  const std::string sasEntryName = sasEntryGroupName + suffix;
  auto sasEntry = H5Util::createGroupCanSAS(file, sasEntryName, nxEntryClassAttr, sasEntryClassAttr);

  // Add version
  H5Util::writeStrAttribute(sasEntry, sasEntryVersionAttr, sasEntryVersionAttrValue);

  // Add definition
  H5Util::write(sasEntry, sasEntryDefinition, sasEntryDefinitionFormat);

  // Add title
  auto const workspaceTitle = workspace->getTitle();
  H5Util::write(sasEntry, sasEntryTitle, workspaceTitle);

  // Add run
  auto runNumber = workspace->getRunNumber();
  H5Util::write(sasEntry, sasEntryRun, std::to_string(runNumber));

  return sasEntry;
}

/**
 * Sorts out dimensionality of the data (1D, 2D) and calls helper
 * function to insert data in workspace to the given sas group.
 * @param group: Handle to the NXcanSAS group
 * @param workspace: the workspace to store data from
 */
void SaveNXcanSASBase::addData(H5::Group &group, const MatrixWorkspace_sptr &workspace) const {
  auto data = H5Util::createGroupCanSAS(group, sasDataGroupName, nxDataClassAttr, sasDataClassAttr);
  auto dim = getWorkspaceDimensionality(workspace);
  areAxesNumeric(workspace, static_cast<int>(dim));
  switch (dim) {
  case (WorkspaceDimensionality::oneD):
    addData1D(data, workspace);
    break;
  case (WorkspaceDimensionality::twoD):
    addData2D(data, workspace);
    break;
  default:
    throw std::runtime_error("SaveNXcanSAS: The provided workspace "
                             "dimensionality is not 1D or 2D.");
  }
}

/**
 * Calls out polarized data function from helper library
 * @param group: Handle to the NXcanSAS group
 * @param wsGroup: Group workspace containing polarized workspaces
 */
void SaveNXcanSASBase::addPolarizedData(H5::Group &group, const WorkspaceGroup_sptr &wsGroup) const {
  auto data = H5Util::createGroupCanSAS(group, sasDataGroupName, nxDataClassAttr, sasDataClassAttr);
  NXcanSAS::addPolarizedData(data, wsGroup, getProperty("InputSpinStates"));
}

/**
 * Saves NXcanSAS data for a group workspace.
 * @param wsGroup: Group workspace containing 1D or 2D SANS polarized workspaces
 * @param path: File system path in which to store the data
 */
void SaveNXcanSASBase::savePolarizedGroup(const WorkspaceGroup_sptr &wsGroup, const std::filesystem::path &path) const {
  auto file = prepareFile(path);
  // Necessary metdata will be taken from first workspace of the group
  auto workspace = std::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(0));
  areAxesNumeric(workspace, static_cast<int>(getWorkspaceDimensionality(workspace)));

  m_progress->report("Adding a new entry.");
  auto sasEntry = addSasEntry(file, workspace, sasEntryDefaultSuffix);

  // Add metadata for canSAS file: Instrument, Sample, Process
  m_progress->report("Adding standard metadata");
  addStandardMetadata(workspace, sasEntry);
  // Add polarized Metadata
  m_progress->report("Adding polarized metadata");
  addPolarizedMetadata(workspace, sasEntry);

  // Add polarized Data
  m_progress->report("Adding polarized data.");
  addPolarizedData(sasEntry, wsGroup);

  file.close();
}

/**
 * Saves NXcanSAS data for a matrix workspace.
 * @param workspace: Matrix workspace containing 1D or 2D SANS data
 * @param path: File system path in which to store the data
 */
void SaveNXcanSASBase::saveSingleWorkspaceFile(const MatrixWorkspace_sptr &workspace,
                                               const std::filesystem::path &path) const {
  auto file = prepareFile(path);

  m_progress->report("Adding a new entry.");
  auto sasEntry = addSasEntry(file, workspace, sasEntryDefaultSuffix);

  // Add metadata for canSAS file: Instrument, Sample, Process
  m_progress->report("Adding standard metadata");
  addStandardMetadata(workspace, sasEntry);

  // Add 1D or 2D data
  m_progress->report("Adding data.");
  addData(sasEntry, workspace);

  file.close();
}

} // namespace Mantid::DataHandling
