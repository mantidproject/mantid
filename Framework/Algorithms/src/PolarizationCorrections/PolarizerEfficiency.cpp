// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizerEfficiency.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string/join.hpp>

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
} // namespace PropertyNames

void PolarizerEfficiency::init() {
  // Declare required input parameters for algorithm and do some validation here
  auto &validator = std::make_shared<CompositeValidator>();
  validator->add<WorkspaceUnitValidator>("Wavelength");
  validator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input, validator),
      "Input group workspace to use for polarization calculation");
  const auto &wavelengthValidator = std::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::ANALYSER_EFFICIENCY, "",
                                                                       Direction::Input, wavelengthValidator),
                  "Analyser efficiency as a function of wavelength");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
      "Polarizer efficiency as a function of wavelength");

  const auto &spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropertyNames::SPIN_STATES, "11,10,01,00", spinValidator,
                  "Order of individual spin states in the input group workspace, e.g. \"01,11,00,10\"");
}

/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an error message
 */
std::map<std::string, std::string> PolarizerEfficiency::validateInputs() {
  std::map<std::string, std::string> errorList;
  const std::string inputWorkspaceName = getProperty(PropertyNames::INPUT_WORKSPACE);
  if (!AnalysisDataService::Instance().doesExist(inputWorkspaceName)) {
    errorList[PropertyNames::INPUT_WORKSPACE] = "The workspace " + inputWorkspaceName + " does not exist in the ADS.";
    return errorList;
  }

  const auto &ws = AnalysisDataService::Instance().retrieve(inputWorkspaceName);
  if (!ws->isGroup()) {
    errorList[PropertyNames::INPUT_WORKSPACE] = "The input workspace is not a group workspace.";
  } else {
    const auto &wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
    if (wsGroup->size() != 4) {
      errorList[PropertyNames::INPUT_WORKSPACE] =
          "The input group workspace must have four periods corresponding to the four spin configurations.";
    }
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

  auto &effCell = convertToHistIfNecessary(
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(getProperty(PropertyNames::ANALYSER_EFFICIENCY)));

  auto &rebin = createChildAlgorithm("RebinToWorkspace");
  rebin->initialize();
  rebin->setProperty("WorkspaceToRebin", effCell);
  rebin->setProperty("WorkspaceToMatch", t00Ws);
  rebin->setProperty("OutputWorkspace", "rebinToWorkspace");
  rebin->execute();
  effCell = rebin->getProperty("OutputWorkspace");

  // The efficiency is given by (e_cell * (T00 + T01) - T01) / ((2 * e_cell - 1) * (T00 + T01))

  const auto &sumT = t00Ws + t01Ws;
  auto &eCellTimesSum = effCell * sumT;
  const auto &numerator = eCellTimesSum - t01Ws;
  eCellTimesSum *= 2;
  const auto &denominator = eCellTimesSum - sumT;
  const auto &effPolarizer = numerator / denominator;
  setProperty(PropertyNames::OUTPUT_WORKSPACE, effPolarizer);
}

MatrixWorkspace_sptr PolarizerEfficiency::convertToHistIfNecessary(const MatrixWorkspace_sptr ws) {
  if (ws->isHistogramData() && ws->isDistribution())
    return ws;

  MatrixWorkspace_sptr wsClone = ws->clone();
  wsClone->setDistribution(true);

  if (wsClone->isHistogramData())
    return wsClone;

  auto &convertToHistogram = createChildAlgorithm("ConvertToHistogram");
  convertToHistogram->initialize();
  convertToHistogram->setProperty("InputWorkspace", wsClone);
  convertToHistogram->setProperty("OutputWorkspace", wsClone);
  convertToHistogram->execute();
  return convertToHistogram->getProperty("OutputWorkspace");
}

} // namespace Mantid::Algorithms