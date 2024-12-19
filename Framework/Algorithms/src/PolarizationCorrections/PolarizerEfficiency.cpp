// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizerEfficiency.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string/join.hpp>
#include <filesystem>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PolarizerEfficiency)

using namespace Kernel;
using namespace API;
using namespace FlipperConfigurations;

namespace PropertyNames {
static const std::string INPUT_WORKSPACE = "InputWorkspace";
static const std::string ANALYSER_EFFICIENCY = "AnalyserEfficiency";
static const std::string SPIN_STATES = "SpinStates";
static const std::string OUTPUT_WORKSPACE = "OutputWorkspace";
static const std::string OUTPUT_FILE_PATH = "OutputFilePath";
} // namespace PropertyNames

namespace {
static const std::string FILE_EXTENSION = ".nxs";

/**
 * Validate the given workspace to ensure it is usable in the corrections.
 * @param ws The workspace to check.
 * @param propertyName The property the workspace came from.
 * @param errorList The list of errors currently found during the validation.
 * @return True if validation can continue afterwards. Stops nullptrs being accessed/checked later in the validation.
 */
bool validateInputWorkspace(MatrixWorkspace_sptr const &ws, std::string const &propertyName,
                            std::map<std::string, std::string> &errorList) {
  if (ws == nullptr) {
    errorList[propertyName] = "All input workspaces must be of type MatrixWorkspace.";
    return false;
  }
  if (ws->getNumberHistograms() != 1) {
    errorList[propertyName] = "All input workspaces must contain a single histogram.";
    return true;
  }
  if (ws->getAxis(0)->unit()->unitID() != "Wavelength") {
    errorList[propertyName] = "All input workspaces must be in units of Wavelength.";
    return true;
  }
  if (!ws->isHistogramData() && ws->isDistribution()) {
    errorList[propertyName] = "All input workspaces must be using distributed histogram data.";
    return true;
  }
  return true;
}

} // unnamed namespace

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

  const auto &spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{2, 3, 4});
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
  // Check input workspaces.
  const WorkspaceGroup_sptr inputWorkspace = getProperty(PropertyNames::INPUT_WORKSPACE);
  if (inputWorkspace == nullptr) {
    errorList[PropertyNames::INPUT_WORKSPACE] = "The input workspace is not a workspace group.";
    return errorList;
  }

  auto const &inputWsCount = inputWorkspace->size();
  if (inputWsCount < 2) {
    errorList[PropertyNames::INPUT_WORKSPACE] =
        "The input group workspace must have at least two periods corresponding to the spin configurations.";
  } else {
    for (size_t i = 0; i < inputWsCount; ++i) {
      const MatrixWorkspace_sptr stateWs = std::dynamic_pointer_cast<MatrixWorkspace>(inputWorkspace->getItem(i));
      if (!validateInputWorkspace(stateWs, PropertyNames::INPUT_WORKSPACE, errorList)) {
        return errorList;
      };
    }
  }
  const MatrixWorkspace_sptr analyserWs = getProperty(PropertyNames::ANALYSER_EFFICIENCY);
  if (!validateInputWorkspace(analyserWs, PropertyNames::ANALYSER_EFFICIENCY, errorList)) {
    return errorList;
  }

  const auto &spinStates =
      PolarizationCorrectionsHelpers::splitSpinStateString(getPropertyValue(PropertyNames::SPIN_STATES));
  if (spinStates.size() != inputWsCount) {
    errorList[PropertyNames::SPIN_STATES] =
        "The number of workspaces in the input WorkspaceGroup (" + std::to_string(inputWsCount) +
        ") does not match the number of spin states provided (" + std::to_string(spinStates.size()) + ").";
    return errorList;
  }
  const auto &t01WsIndex =
      PolarizationCorrectionsHelpers::indexOfWorkspaceForSpinState(spinStates, FlipperConfigurations::OFF_ON);
  const auto &t00WsIndex =
      PolarizationCorrectionsHelpers::indexOfWorkspaceForSpinState(spinStates, FlipperConfigurations::OFF_OFF);
  if (!t01WsIndex.has_value() || !t00WsIndex.has_value()) {
    errorList[PropertyNames::SPIN_STATES] =
        "The required spin configurations (00, 01) could not be found in the given SpinStates.";
  } else {
    const MatrixWorkspace_sptr t00Ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(inputWorkspace->getItem(t00WsIndex.value()));
    if (!WorkspaceHelpers::matchingBins(*t00Ws, *analyserWs, true)) {
      errorList[PropertyNames::ANALYSER_EFFICIENCY] = "The bins in the " + std::string(PropertyNames::INPUT_WORKSPACE) +
                                                      " and " + PropertyNames::ANALYSER_EFFICIENCY +
                                                      "workspace do not match.";
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

void PolarizerEfficiency::exec() { calculatePolarizerEfficiency(); }

void PolarizerEfficiency::calculatePolarizerEfficiency() {
  // First we extract the individual workspaces corresponding to each spin configuration from the group workspace
  const WorkspaceGroup_sptr &groupWorkspace = getProperty(PropertyNames::INPUT_WORKSPACE);
  const auto spinConfigurationInput = getPropertyValue(PropertyNames::SPIN_STATES);

  const auto &t01Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                            FlipperConfigurations::OFF_ON);
  const auto &t00Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                            FlipperConfigurations::OFF_OFF);

  const MatrixWorkspace_sptr effCell = getProperty(PropertyNames::ANALYSER_EFFICIENCY);

  auto &&effPolarizer = (t00Ws - t01Ws) / (2 * (2 * effCell - 1) * (t00Ws + t01Ws)) + 0.5;

  calculateErrors(t00Ws, t01Ws, effCell, effPolarizer);

  auto const &filename = getPropertyValue(PropertyNames::OUTPUT_FILE_PATH);
  if (!filename.empty()) {
    saveToFile(effPolarizer, filename);
  }

  auto const &outputWsName = getPropertyValue(PropertyNames::OUTPUT_WORKSPACE);
  if (!outputWsName.empty()) {
    setProperty(PropertyNames::OUTPUT_WORKSPACE, effPolarizer);
  }
}

void PolarizerEfficiency::calculateErrors(const MatrixWorkspace_sptr &t00Ws, const MatrixWorkspace_sptr &t01Ws,
                                          const MatrixWorkspace_sptr &effCellWs,
                                          const MatrixWorkspace_sptr &effPolarizerWs) {

  auto &effPolarizerE = effPolarizerWs->mutableE(0);
  const auto &t00E = t00Ws->e(0);
  const auto &t01E = t01Ws->e(0);
  const auto &effCellE = effCellWs->e(0);
  const auto &t00Y = t00Ws->y(0);
  const auto &t01Y = t01Ws->y(0);
  const auto &effCellY = effCellWs->y(0);

  for (size_t i = 0; i < effPolarizerE.size(); ++i) {
    const auto &twoCellEffMinusOne = 2 * effCellY[i] - 1;
    const auto &t00PlusT01 = t00Y[i] + t01Y[i];

    const auto &delta00 = (t01Y[i]) / ((twoCellEffMinusOne)*pow(t00PlusT01, 2));
    const auto &delta01 = (-1 * t00Y[i]) / ((twoCellEffMinusOne)*pow(t00PlusT01, 2));
    const auto &deltaEffCell = (t01Y[i] - t00Y[i]) / (pow(twoCellEffMinusOne, 2) * (t00PlusT01));
    effPolarizerE[i] = sqrt(pow(delta00 * t00E[i], 2) + pow(delta01 * t01E[i], 2) + pow(deltaEffCell * effCellE[i], 2));
  }
}

void PolarizerEfficiency::saveToFile(MatrixWorkspace_sptr const &workspace, std::string const &filePathStr) {
  std::filesystem::path filePath = filePathStr;
  // Add the nexus extension if it's not been applied already.
  if (filePath.extension() != FILE_EXTENSION) {
    filePath.replace_extension(FILE_EXTENSION);
  }
  auto saveAlg = createChildAlgorithm("SaveNexus");
  saveAlg->initialize();
  saveAlg->setProperty("Filename", filePath.string());
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->execute();
}

} // namespace Mantid::Algorithms
