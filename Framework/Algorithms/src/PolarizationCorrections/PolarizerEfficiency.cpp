// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizerEfficiency.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"

#include <boost/algorithm/string/join.hpp>
#include <filesystem>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PolarizerEfficiency)

using namespace Kernel;
using namespace API;

namespace PropertyNames {
static const std::string INPUT_WORKSPACE = "InputWorkspace";
static const std::string ANALYSER_EFFICIENCY = "AnalyserEfficiency";
static const std::string SPIN_STATES = "SpinStates";
static const std::string OUTPUT_WORKSPACE = "OutputWorkspace";
static const std::string OUTPUT_FILE_PATH = "OutputFilePath";
} // namespace PropertyNames

void PolarizerEfficiency::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input),
      "Input group workspace to use for polarization calculation");
  const auto &wavelengthValidator = std::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::ANALYSER_EFFICIENCY, "",
                                                                       Direction::Input, wavelengthValidator),
                  "Analyser efficiency as a function of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WORKSPACE, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Polarizer efficiency as a function of wavelength");

  const auto &spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropertyNames::SPIN_STATES, "11,10,01,00", spinValidator,
                  "Order of individual spin states in the input group workspace, e.g. \"01,11,00,10\"");

  declareProperty(std::make_unique<FileProperty>(PropertyNames::OUTPUT_FILE_PATH, "", FileProperty::OptionalSave),
                  "File name or path for the output to be saved to.");
}

/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an error message
 */
std::map<std::string, std::string> PolarizerEfficiency::validateInputs() {
  std::map<std::string, std::string> errorList;
  const WorkspaceGroup_sptr inputWorkspace = getProperty(PropertyNames::INPUT_WORKSPACE);
  if (inputWorkspace == nullptr) {
    errorList[PropertyNames::INPUT_WORKSPACE] = "The input workspace is not a workspace group.";
    return errorList;
  }

  if (inputWorkspace->size() != 4) {
    errorList[PropertyNames::INPUT_WORKSPACE] =
        "The input group workspace must have four periods corresponding to the four spin configurations.";
  }

  for (size_t i = 0; i < inputWorkspace->size(); ++i) {
    const MatrixWorkspace_sptr stateWs = std::dynamic_pointer_cast<MatrixWorkspace>(inputWorkspace->getItem(i));
    Unit_const_sptr unit = stateWs->getAxis(0)->unit();
    if (unit->unitID() != "Wavelength") {
      errorList[PropertyNames::INPUT_WORKSPACE] = "All input workspaces must be in units of Wavelength.";
    }
  }

  // Check outputs.
  auto const &outputWs = getPropertyValue(PropertyNames::OUTPUT_WORKSPACE);
  auto const &outputFile = getPropertyValue(PropertyNames::OUTPUT_FILE_PATH);
  if (outputWs.empty() && outputFile.empty()) {
    errorList[PropertyNames::OUTPUT_FILE_PATH] = "Either an output workspace or output file must be provided.";
    errorList[PropertyNames::OUTPUT_WORKSPACE] = "Either an output workspace or output file must be provided.";
  }

  return errorList;
}

/**
 * Explicitly calls validateInputs and throws runtime error in case
 * of issues in the input properties.
 */
void PolarizerEfficiency::validateGroupInput() {
  const auto &results = validateInputs();
  if (results.size() > 0) {
    const auto &result = results.cbegin();
    throw std::runtime_error("Issue in " + result->first + " property: " + result->second);
  }
}

bool PolarizerEfficiency::processGroups() {
  validateGroupInput();
  calculatePolarizerEfficiency();
  return true;
}

void PolarizerEfficiency::exec() { calculatePolarizerEfficiency(); }

void PolarizerEfficiency::calculatePolarizerEfficiency() {
  // First we extract the individual workspaces corresponding to each spin configuration from the group workspace
  const auto &groupWorkspace =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getProperty(PropertyNames::INPUT_WORKSPACE));
  const std::string spinConfigurationInput = getProperty(PropertyNames::SPIN_STATES);

  const auto &t01Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                            SpinStateValidator::ZERO_ONE);
  const auto &t00Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                            SpinStateValidator::ZERO_ZERO);

  auto effCell = convertToHistIfNecessary(
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(getProperty(PropertyNames::ANALYSER_EFFICIENCY)));

  auto rebin = createChildAlgorithm("RebinToWorkspace");
  rebin->initialize();
  rebin->setProperty("WorkspaceToRebin", effCell);
  rebin->setProperty("WorkspaceToMatch", t00Ws);
  rebin->setProperty("OutputWorkspace", "rebinToWorkspace");
  rebin->execute();
  effCell = rebin->getProperty("OutputWorkspace");

  const auto &effPolarizer = (t00Ws - t01Ws) / (4 * (2 * effCell - 1) * (t00Ws + t01Ws)) + 0.5;

  auto const &filename = getPropertyValue(PropertyNames::OUTPUT_FILE_PATH);
  if (!filename.empty()) {
    saveToFile(effPolarizer, filename);
  }

  auto const &outputWsName = getPropertyValue(PropertyNames::OUTPUT_WORKSPACE);
  if (!outputWsName.empty()) {
    setProperty(PropertyNames::OUTPUT_WORKSPACE, effPolarizer);
  }
}

void PolarizerEfficiency::saveToFile(MatrixWorkspace_sptr const &workspace, std::string const &filePathStr) {
  std::filesystem::path filePath = filePathStr;
  // Add the nexus extension if it's not been applied already.
  const std::string fileExtension = ".nxs";
  if (filePath.extension() != fileExtension) {
    filePath.replace_extension(fileExtension);
  }
  auto saveAlg = createChildAlgorithm("SaveNexus");
  saveAlg->initialize();
  saveAlg->setProperty("Filename", filePath.string());
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->execute();
}

MatrixWorkspace_sptr PolarizerEfficiency::convertToHistIfNecessary(const MatrixWorkspace_sptr ws) {
  if (ws->isHistogramData() && ws->isDistribution())
    return ws;

  MatrixWorkspace_sptr wsClone = ws->clone();
  wsClone->setDistribution(true);

  if (wsClone->isHistogramData())
    return wsClone;

  auto convertToHistogram = createChildAlgorithm("ConvertToHistogram");
  convertToHistogram->initialize();
  convertToHistogram->setProperty("InputWorkspace", wsClone);
  convertToHistogram->setProperty("OutputWorkspace", wsClone);
  convertToHistogram->execute();
  return convertToHistogram->getProperty("OutputWorkspace");
}

} // namespace Mantid::Algorithms