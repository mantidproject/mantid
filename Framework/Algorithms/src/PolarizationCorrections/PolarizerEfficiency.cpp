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

namespace PropertyNames {
static const std::string INPUT_WORKSPACE = "InputWorkspace";
static const std::string ANALYSER_EFFICIENCY = "AnalyserEfficiency";
static const std::string SPIN_STATES = "SpinStates";
static const std::string OUTPUT_WORKSPACE = "OutputWorkspace";
static const std::string OUTPUT_FILE_PATH = "OutputFilePath";
} // namespace PropertyNames

namespace {
static const std::string FILE_EXTENSION = ".nxs";

void validateInputWorkspace(MatrixWorkspace_sptr const &ws, std::string const &propertyName,
                            std::map<std::string, std::string> &errorList) {
  if (ws == nullptr) {
    errorList[propertyName] = "All input workspaces must be of type MatrixWorkspace.";
    return;
  }
  if (ws->getNumberHistograms() != 1) {
    errorList[propertyName] = "All input workspaces must contain a single histogram.";
  }
  if (ws->getAxis(0)->unit()->unitID() != "Wavelength") {
    errorList[propertyName] = "All input workspaces must be in units of Wavelength.";
  }
  if (!ws->isHistogramData() && ws->isDistribution()) {
    errorList[propertyName] = "All input workspaces must be using distributed histogram data.";
  }
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
  if (inputWsCount < 2 || inputWsCount > 4) {
    errorList[PropertyNames::INPUT_WORKSPACE] =
        "The input group workspace must have at least two periods corresponding to the spin configurations.";
  } else {
    for (size_t i = 0; i < inputWsCount; ++i) {
      const MatrixWorkspace_sptr stateWs = std::dynamic_pointer_cast<MatrixWorkspace>(inputWorkspace->getItem(i));
      validateInputWorkspace(stateWs, PropertyNames::INPUT_WORKSPACE, errorList);
    }
  }
  const MatrixWorkspace_sptr analyserWs = getProperty(PropertyNames::ANALYSER_EFFICIENCY);
  validateInputWorkspace(analyserWs, PropertyNames::ANALYSER_EFFICIENCY, errorList);

  const auto &spinConfigurationInput = getPropertyValue(PropertyNames::SPIN_STATES);
  const auto &spinStateCount = PolarizationCorrectionsHelpers::splitSpinStateString(spinConfigurationInput).size();
  if (spinStateCount != inputWsCount) {
    errorList[PropertyNames::SPIN_STATES] =
        "The number of workspaces in the input WorkspaceGroup ( " + std::to_string(inputWsCount) +
        ") does not match the number of spin states provided (" + std::to_string(spinStateCount) + ").";
  }
  const auto &t01Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(inputWorkspace, spinConfigurationInput,
                                                                            SpinStateValidator::ZERO_ONE);
  const auto &t00Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(inputWorkspace, spinConfigurationInput,
                                                                            SpinStateValidator::ZERO_ZERO);
  if (t01Ws == nullptr || t00Ws == nullptr) {
    errorList[PropertyNames::SPIN_STATES] = "The required spin configurations (00, 01) could not be found in the given"
                                            "SpinStates.";
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
                                                                            SpinStateValidator::ZERO_ONE);
  const auto &t00Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                            SpinStateValidator::ZERO_ZERO);

  MatrixWorkspace_sptr effCell = getProperty(PropertyNames::ANALYSER_EFFICIENCY);

  auto rebin = createChildAlgorithm("RebinToWorkspace");
  rebin->initialize();
  rebin->setProperty("WorkspaceToRebin", effCell);
  rebin->setProperty("WorkspaceToMatch", t00Ws);
  rebin->setProperty("OutputWorkspace", "rebinToWorkspace");
  rebin->execute();
  effCell = rebin->getProperty("OutputWorkspace");

  auto &&effPolarizer = (t00Ws - t01Ws) / (4 * (2 * effCell - 1) * (t00Ws + t01Ws)) + 0.5;

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
    const auto &delta00 = (t01Y[i]) / (2 * (2 * effCellY[i] - 1) * pow(t00Y[i] + t01Y[i], 2));
    const auto &delta01 = (-1 * t00Y[i]) / (2 * (2 * effCellY[i] - 1) * pow(t00Y[i] + t01Y[i], 2));
    const auto &deltaEffCell = (t01Y[i] - t00Y[i]) / (2 * pow(2 * effCellY[i] - 1, 2) * (t00Y[i] + t01Y[i]));
    effPolarizerE[i] = sqrt(pow(abs(delta00), 2) * pow(t00E[i], 2) + pow(abs(delta01), 2) * pow(t01E[i], 2) +
                            pow(abs(deltaEffCell), 2) * pow(effCellE[i], 2));
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
