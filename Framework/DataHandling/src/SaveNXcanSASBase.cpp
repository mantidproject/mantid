// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNXcanSASBase.h"
#include "MantidAPI//WorkspaceGroup.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataHandling/NXcanSASHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/LambdaValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"

#include <MantidAPI/AlgorithmRuntimeProps.h>
#include <MantidKernel/VectorHelper.h>
#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::Kernel::VectorHelper;

namespace {

/// Property names.
namespace SpinState {
static const std::string SPIN_PARA = "-1";
static const std::string SPIN_ANTIPARA = "+1";
static const std::string SPIN_ZERO = "0";
} // namespace SpinState

namespace PolProperties {
static const std::string INPUT_SPIN_STATES = "InputSpinStates";
static const std::string POLARIZER_COMP_NAME = "PolarizerComponentName";
static const std::string ANALYZER_COMP_NAME = "AnalyzerComponentName";
static const std::string FLIPPER_COMP_NAMES = "FlipperComponentNames";
static const std::string MAG_FIELD_STRENGTH_LOGNAME = "MagneticFieldStrengthLogName";
static const std::string MAG_FIELD_DIR = "MagneticFieldDirection";
std::map<std::string, std::string> POL_COMPONENTS = {
    {POLARIZER_COMP_NAME, "polarizer"}, {ANALYZER_COMP_NAME, "analyzer"}, {FLIPPER_COMP_NAMES, "flipper"}};

} // namespace PolProperties

bool hasUnit(const std::string &unitToCompareWith, const MatrixWorkspace_sptr &ws) {
  if (ws->axes() == 0) {
    return false;
  }
  auto const unit = ws->getAxis(0)->unit();
  return (unit && !unit->unitID().compare(unitToCompareWith));
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
    auto const &groupItems = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws)->getAllItems();
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
  declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Workspace>>("InputWorkspace", "",
                                                                              Kernel::Direction::Input, groupValidator),
                  "The input workspace, which must be in units of Q. Can be a 1D or a 2D workspace.");
  declareProperty(std::make_unique<Mantid::API::FileProperty>("Filename", "", API::FileProperty::Save, ".h5"),
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
  declareProperty("RadiationSource", "Spallation Neutron Source",
                  std::make_shared<Kernel::StringListValidator>(radiationSourceOptions), "The type of radiation used.");
  declareProperty("DetectorNames", "",
                  "Specify in a comma separated list, which detectors to store "
                  "information about; \nwhere each name must match a name "
                  "given for a detector in the [[IDF|instrument definition "
                  "file (IDF)]]. \nIDFs are located in the instrument "
                  "sub-directory of the Mantid install directory.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<>>("Transmission", "", Kernel::Direction::Input, PropertyMode::Optional,
                                                 std::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
      "The transmission workspace. Optional. If given, will be saved at "
      "TransmissionSpectrum");

  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "TransmissionCan", "", Kernel::Direction::Input, PropertyMode::Optional,
                      std::make_shared<API::WorkspaceUnitValidator>("Wavelength")),
                  "The transmission workspace of the Can. Optional. If given, will be "
                  "saved at TransmissionSpectrum");

  declareProperty("SampleTransmissionRunNumber", "", "The run number for the sample transmission workspace. Optional.");
  declareProperty("SampleDirectRunNumber", "", "The run number for the sample direct workspace. Optional.");
  declareProperty("CanScatterRunNumber", "", "The run number for the can scatter workspace. Optional.");
  declareProperty("CanDirectRunNumber", "", "The run number for the can direct workspace. Optional.");

  declareProperty(
      "BackgroundSubtractionWorkspace", "",
      "The name of the workspace used in the scaled background subtraction, to be included in the metadata. Optional.");
  declareProperty(
      "BackgroundSubtractionScaleFactor", 0.0,
      "The scale factor used in the scaled background subtraction, to be included in the metadata. Optional.");

  std::vector<std::string> const geometryOptions{"Cylinder", "FlatPlate", "Flat plate", "Disc", "Unknown"};
  declareProperty("Geometry", "Unknown", std::make_shared<Kernel::StringListValidator>(geometryOptions),
                  "The geometry type of the collimation.");
  declareProperty("SampleHeight", 0.0,
                  "The height of the collimation element in mm. If specified as 0 it will not be recorded.");
  declareProperty("SampleWidth", 0.0,
                  "The width of the collimation element in mm. If specified as 0 it will not be recorded.");
  declareProperty("SampleThickness", 0.0,
                  "The thickness of the sample in mm. If specified as 0 it will not be recorded.");
}

void SaveNXcanSASBase::initPolarizedProperties() {
  auto val2 = std::make_shared<Algorithms::SpinStateValidator>(std::unordered_set<int>{2, 3});
  const auto spinStateValidator = std::make_shared<Algorithms::SpinStateValidator>(
      std::unordered_set<int>{2, 4}, true, SpinState::SPIN_PARA, SpinState::SPIN_ANTIPARA, true, SpinState::SPIN_ZERO);

  declareProperty(PolProperties::INPUT_SPIN_STATES, "", spinStateValidator,
                  "The order of the spin states in the input group workspace");
  declareProperty(PolProperties::POLARIZER_COMP_NAME, "",
                  "The name of the Polarizer Component. i.e. 'short-polarizer'");
  declareProperty(PolProperties::ANALYZER_COMP_NAME, "", "The name of the Analyzer Component. i.e. 'helium-analyzer'");
  declareProperty(PolProperties::FLIPPER_COMP_NAMES, "", "Comma separated list of flipper components i.e. 'RF-flipper");
  declareProperty(PolProperties::MAG_FIELD_STRENGTH_LOGNAME, "",
                  "The name of the magnetic field strength field recorded on the sample logs");
  declareProperty(PolProperties::MAG_FIELD_DIR, "",
                  "Direction of the magnetic field on the sample, comma separated strings"
                  "with three values: polar, azimuthal and rotation angles");
}

std::map<std::string, std::string> SaveNXcanSASBase::validateStandardInputs() {
  std::map<std::string, std::string> result;

  /* If input workspace is a group, check that each group members is a valid 2D Workspace,
     otherwise check that the input is a valid 2D workspace.*/
  Mantid::API::Workspace_sptr const workspace = getProperty("InputWorkspace");
  auto valid2DWorkspace = [](const Workspace_sptr &ws) {
    return ws && std::dynamic_pointer_cast<const Mantid::DataObjects::Workspace2D>(ws);
  };
  if (workspace->isGroup()) {
    auto const &groupItems = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)->getAllItems();
    if (std::any_of(groupItems.cbegin(), groupItems.cend(),
                    [&](auto const &childWs) { return !valid2DWorkspace(childWs); })) {
      result.emplace("InputWorkspace", "All input workspaces in the input group must be a Workspace2D.");
    }
  } else {
    if (!valid2DWorkspace(workspace)) {
      result.emplace("InputWorkspace", "The InputWorkspace must be a Workspace2D.");
    }
  }

  // Transmission data should be 1D
  Mantid::API::MatrixWorkspace_sptr transmission = getProperty("Transmission");
  Mantid::API::MatrixWorkspace_sptr transmissionCan = getProperty("TransmissionCan");

  auto checkTransmission = [&result](const Mantid::API::MatrixWorkspace_sptr &trans, const std::string &propertyName) {
    if (trans->getNumberHistograms() != 1) {
      result.emplace(propertyName, "The input workspaces for transmissions have to be 1D.");
    }
  };

  if (transmission) {
    checkTransmission(transmission, "TransmissionCan");
  }
  if (transmissionCan) {
    checkTransmission(transmissionCan, "TransmissionCan");
  }

  return result;
}
std::map<std::string, std::string>
SaveNXcanSASBase::validatePolarizedInputs(std::map<std::string, std::string> &result) {
  Mantid::API::Workspace_sptr const workspace = getProperty("InputWorkspace");
  if (!workspace->isGroup())
    result.emplace("InputWorkspace", "Input Workspaces for polarized data can only be workspace groups.");
  else {
    auto const wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
    auto const &entries = wsGroup->getNumberOfEntries();
    if (entries != 2 && entries != 4) {
      result.emplace("InputWorkspace", "Input Group Workspace can only contain 2 or 4 workspace members.");
    }
    std::string const spins = getProperty("InputSpinStates");
    if (entries != static_cast<int>(splitStringIntoVector<std::string>(spins).size())) {
      result.emplace(PolProperties::INPUT_SPIN_STATES, "The number of spin states is different than the number of"
                                                       " member workspaces on the InputWorkspace group");
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

void SaveNXcanSASBase::addStandardMetadata(const MatrixWorkspace_sptr &workspace, H5::Group &sasEntry) {
  std::string &&radiationSource = getPropertyValue("RadiationSource");
  std::string &&geometry = getProperty("Geometry");
  double &&beamHeight = getProperty("SampleHeight");
  double &&beamWidth = getProperty("SampleWidth");
  double &&sampleThickness = getProperty("SampleThickness");
  std::string &&detectorNames = getPropertyValue("DetectorNames");

  Mantid::API::MatrixWorkspace_sptr &&transmissionSample = getProperty("Transmission");
  Mantid::API::MatrixWorkspace_sptr &&transmissionCan = getProperty("TransmissionCan");

  // Add the instrument information
  const auto detectors = Kernel::VectorHelper::splitStringIntoVector<std::string>(detectorNames);
  addInstrument(sasEntry, workspace, radiationSource, geometry, beamHeight, beamWidth, detectors);

  // Add the sample information
  addSample(sasEntry, sampleThickness);

  // Get additional run numbers
  const auto sampleTransmissionRun = getPropertyValue("SampleTransmissionRunNumber");
  const auto sampleDirectRun = getPropertyValue("SampleDirectRunNumber");
  const auto canScatterRun = getPropertyValue("CanScatterRunNumber");
  const auto canDirectRun = getPropertyValue("CanDirectRunNumber");

  // Get scaled background subtraction information

  const auto scaledBgSubWorkspace = getPropertyValue("BackgroundSubtractionWorkspace");
  const auto scaledBgSubScaleFactor = getPropertyValue("BackgroundSubtractionScaleFactor");

  // Add the process information
  if (transmissionCan) {
    addProcess(sasEntry, workspace, transmissionCan);
  } else {
    addProcess(sasEntry, workspace);
  }

  if (transmissionCan) {
    addProcessEntry(sasEntry, sasProcessTermCanScatter, canScatterRun);
    addProcessEntry(sasEntry, sasProcessTermCanDirect, canDirectRun);
  }
  if (transmissionSample) {
    addProcessEntry(sasEntry, sasProcessTermSampleTrans, sampleTransmissionRun);
    addProcessEntry(sasEntry, sasProcessTermSampleDirect, sampleDirectRun);
  }

  if (!scaledBgSubWorkspace.empty()) {
    addProcessEntry(sasEntry, sasProcessTermScaledBgSubWorkspace, scaledBgSubWorkspace);
    addProcessEntry(sasEntry, sasProcessTermScaledBgSubScaleFactor, scaledBgSubScaleFactor);
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

void SaveNXcanSASBase::addPolarizedMetadata(const MatrixWorkspace_sptr &workspace, H5::Group &sasEntry) {

  for (auto const &[compName, compType] : createComponentMap()) {
    addPolarizer(sasEntry, workspace, compName, compType);
  }
  std::string const &sasSampleMagneticField = getProperty(PolProperties::MAG_FIELD_STRENGTH_LOGNAME);
  if (!sasSampleMagneticField.empty()) {
    std::string const &magneticFieldDirStr = getProperty(PolProperties::MAG_FIELD_DIR);
    addSampleEMFields(sasEntry, workspace, sasSampleMagneticField, magneticFieldDirStr);
  }
}

/**
 * Add the sasEntry to the sasroot.
 * @param file: Handle to the NXcanSAS file
 * @param workspace: the workspace to store
 * @param suffix: suffix for sas entry group name. Default is "01"
 * @return sasEntry group object
 */
H5::Group SaveNXcanSASBase::addSasEntry(H5::H5File &file, const Mantid::API::MatrixWorkspace_sptr &workspace,
                                        const std::string &suffix) {
  const std::string sasEntryName = sasEntryGroupName + suffix;
  auto sasEntry = Mantid::NeXus::H5Util::createGroupCanSAS(file, sasEntryName, nxEntryClassAttr, sasEntryClassAttr);

  // Add version
  Mantid::NeXus::H5Util::writeStrAttribute(sasEntry, sasEntryVersionAttr, sasEntryVersionAttrValue);

  // Add definition
  Mantid::NeXus::H5Util::write(sasEntry, sasEntryDefinition, sasEntryDefinitionFormat);

  // Add title
  auto workspaceTitle = workspace->getTitle();
  Mantid::NeXus::H5Util::write(sasEntry, sasEntryTitle, workspaceTitle);

  // Add run
  const auto runNumber = workspace->getRunNumber();
  Mantid::NeXus::H5Util::write(sasEntry, sasEntryRun, std::to_string(runNumber));

  return sasEntry;
}

/**
 * Sorts out dimensionality of the data (1D, 2D) and calls helper
 * function to insert data in workspace to the given sas group.
 * @param group: Handle to the NXcanSAS group
 * @param workspace: the workspace to store data from
 */
void SaveNXcanSASBase::addData(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace) {
  auto data = Mantid::NeXus::H5Util::createGroupCanSAS(group, sasDataGroupName, nxDataClassAttr, sasDataClassAttr);

  switch (getWorkspaceDimensionality(workspace)) {
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
 * Sorts out dimensionality of data and spin states and calls addPolarizedData
 * @param group: Handle to the NXcanSAS group
 * @param wsGroup: Group workspace containing polarized workspaces
 */
void SaveNXcanSASBase::addPolarizedData(H5::Group &group, const Mantid::API::WorkspaceGroup_sptr &wsGroup) {
  auto data = Mantid::NeXus::H5Util::createGroupCanSAS(group, sasDataGroupName, nxDataClassAttr, sasDataClassAttr);
  NXcanSAS::addPolarizedData(data, wsGroup, getProperty("InputSpinStates"));
}

void SaveNXcanSASBase::savePolarizedGroup(const WorkspaceGroup_sptr &wsGroup, const std::filesystem::path &path) {
  // Prepare file
  if (!path.empty()) {
    std::filesystem::remove(path);
  }
  H5::H5File file(path.string(), H5F_ACC_EXCL, NeXus::H5Util::defaultFileAcc());

  // Necessary metdata will be taken from first workspace of the group
  auto workspace = std::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(0));

  const std::string suffix("01");
  m_progress->report("Adding a new entry.");
  auto sasEntry = addSasEntry(file, workspace, suffix);

  // Add metadata for canSAS file: Instrument, Sample, Process
  m_progress->report("Adding standard metadata");
  addStandardMetadata(workspace, sasEntry);
  addPolarizedMetadata(workspace, sasEntry);

  m_progress->report("Adding data.");
  addPolarizedData(sasEntry, wsGroup);

  file.close();
}

void SaveNXcanSASBase::saveSingleWorkspaceFile(const API::MatrixWorkspace_sptr &workspace,
                                               const std::filesystem::path &path) {
  // Prepare file
  if (!path.empty()) {
    std::filesystem::remove(path);
  }
  H5::H5File file(path.string(), H5F_ACC_EXCL, NeXus::H5Util::defaultFileAcc());

  const std::string suffix("01");
  m_progress->report("Adding a new entry.");
  auto sasEntry = addSasEntry(file, workspace, suffix);

  // Add metadata for canSAS file: Instrument, Sample, Process
  m_progress->report("Adding standard metadata");
  addStandardMetadata(workspace, sasEntry);

  m_progress->report("Adding data.");
  addData(sasEntry, workspace);

  file.close();
}

std::map<std::string, std::string> SaveNXcanSASBase::createComponentMap() {
  // assumption here is that we will pass the IDFcomponent names as comma separated lists
  std::map<std::string, std::string> componentMap;
  for (auto const &[compName, compType] : PolProperties::POL_COMPONENTS) {
    std::string componentName = getProperty(compName);
    if (!componentName.empty()) {
      auto const components = splitStringIntoVector<std::string>(componentName);
      for (auto const &component : components) {
        componentMap.emplace(std::make_pair(component, compType));
      }
    }
  }
  return componentMap;
}

} // namespace Mantid::DataHandling
