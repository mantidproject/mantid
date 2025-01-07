// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNXcanSASBase.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataHandling/NXcanSASHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidNexus/H5Util.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;

namespace Mantid::DataHandling {

void SaveNXcanSASBase::initStandardProperties() {
  // Standard NXcanSAS properties
  auto inputWSValidator = std::make_shared<Kernel::CompositeValidator>();
  inputWSValidator->add<API::WorkspaceUnitValidator>("MomentumTransfer");
  inputWSValidator->add<API::CommonBinsValidator>();
  declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<>>("InputWorkspace", "", Kernel::Direction::Input,
                                                                     inputWSValidator),
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

std::map<std::string, std::string> SaveNXcanSASBase::validateStandardInputs() {
  // The input should be a Workspace2D
  Mantid::API::MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  std::map<std::string, std::string> result;
  if (!workspace || !std::dynamic_pointer_cast<const Mantid::DataObjects::Workspace2D>(workspace)) {
    result.emplace("InputWorkspace", "The InputWorkspace must be a Workspace2D.");
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
  const auto detectors = splitDetectorNames(detectorNames);
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

} // namespace Mantid::DataHandling
