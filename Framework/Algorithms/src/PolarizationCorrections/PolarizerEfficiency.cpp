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
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string/join.hpp>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PolarizerEfficiency)

using namespace Kernel;
using namespace API;

namespace SpinConfigurations {
static const std::string UP_UP = "11";
static const std::string UP_DOWN = "10";
static const std::string DOWN_UP = "01";
static const std::string DOWN_DOWN = "00";
} // namespace SpinConfigurations

namespace PropertyNames {
static const std::string INPUT_WORKSPACE = "InputWorkspace";
static const std::string ANALYSER_EFFICIENCY = "AnalyserEfficiency";
static const std::string SPIN_STATES = "SpinStates";
static const std::string OUTPUT_WORKSPACE = "OutputWorkspace";
} // namespace PropertyNames

void PolarizerEfficiency::init() {
  // Declare required input parameters for algorithm and do some validation here
  auto validator = std::make_shared<CompositeValidator>();
  validator->add<WorkspaceUnitValidator>("Wavelength");
  validator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input, validator),
      "Input group workspace to use for polarization calculation");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::ANALYSER_EFFICIENCY, "",
                                                                       Direction::Input, validator),
                  "Analyser efficiency as a function of wavelength");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
      "Polarizer efficiency as a function of wavelength");

  std::vector<std::string> initialSpinConfig{{SpinConfigurations::UP_UP, SpinConfigurations::UP_DOWN,
                                              SpinConfigurations::DOWN_UP, SpinConfigurations::DOWN_DOWN}};
  std::sort(initialSpinConfig.begin(), initialSpinConfig.end());
  std::vector<std::string> allowedSpinConfigs;
  allowedSpinConfigs.push_back(boost::algorithm::join(initialSpinConfig, ","));
  while (std::next_permutation(initialSpinConfig.begin(), initialSpinConfig.end())) {
    allowedSpinConfigs.push_back(boost::algorithm::join(initialSpinConfig, ","));
  }
  // "Order of individual spin states in the input group workspace, e.g. \"01,11,00,10\""
  declareProperty(
      PropertyNames::SPIN_STATES,
      boost::algorithm::join(std::vector<std::string>{{SpinConfigurations::UP_UP, SpinConfigurations::DOWN_UP,
                                                       SpinConfigurations::DOWN_DOWN, SpinConfigurations::UP_DOWN}},
                             ","),
      std::make_shared<ListValidator<std::string>>(allowedSpinConfigs));
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

  const auto ws = AnalysisDataService::Instance().retrieve(inputWorkspaceName);
  if (!ws->isGroup()) {
    errorList[PropertyNames::INPUT_WORKSPACE] = "The input workspace is not a group workspace.";
  } else {
    const auto wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
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
  const auto results = validateInputs();
  for (const auto &result : results) {
    throw std::runtime_error("Issue in " + result.first + " property: " + result.second);
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
  const auto groupWorkspace =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getProperty(PropertyNames::INPUT_WORKSPACE));
  std::string spinConfigurationInput = getProperty(PropertyNames::SPIN_STATES);
  std::vector<std::string> spinConfigurations;
  boost::split(spinConfigurations, spinConfigurationInput, boost::is_any_of(","));

  const auto t01Ws = workspaceForSpinConfig(groupWorkspace, spinConfigurations, SpinConfigurations::DOWN_UP);
  const auto t00Ws = workspaceForSpinConfig(groupWorkspace, spinConfigurations, SpinConfigurations::DOWN_DOWN);

  const MatrixWorkspace_sptr effCell =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(getProperty(PropertyNames::ANALYSER_EFFICIENCY));

  // The efficiency is given by 0.5 + (T_00 - T_01) / (8 * e_cell - 4)

  auto minus = createChildAlgorithm("Minus");
  minus->initialize();
  minus->setProperty("LHSWorkspace", t00Ws);
  minus->setProperty("RHSWorkspace", t01Ws);
  minus->setProperty("OutputWorkspace", "numerator");
  minus->execute();
  MatrixWorkspace_sptr numerator = minus->getProperty("OutputWorkspace");

  // To divide workspaces they need to have matching bins
  auto rebinToWorkspace = createChildAlgorithm("RebinToWorkspace");
  rebinToWorkspace->initialize();
  rebinToWorkspace->setProperty("WorkspaceToRebin", effCell);
  rebinToWorkspace->setProperty("WorkspaceToMatch", numerator);
  rebinToWorkspace->setProperty("OutputWorkspace", "effCellRebinned");
  rebinToWorkspace->execute();
  MatrixWorkspace_sptr denominator = rebinToWorkspace->getProperty("OutputWorkspace");

  scaleWorkspace(denominator, 8);
  addOffsetToWorkspace(denominator, -4);

  auto divide = createChildAlgorithm("Divide");
  divide->initialize();
  divide->setProperty("LHSWorkspace", numerator);
  divide->setProperty("RHSWorkspace", denominator);
  divide->setProperty("OutputWorkspace", "effPolarizer");
  divide->execute();
  MatrixWorkspace_sptr effPolarizer = divide->getProperty("OutputWorkspace");

  addOffsetToWorkspace(effPolarizer, 0.5);

  setProperty(PropertyNames::OUTPUT_WORKSPACE, effPolarizer);
}

void PolarizerEfficiency::runScaleAlgorithm(MatrixWorkspace_sptr ws, const double factor, const bool isMultiply) {
  auto scale = createChildAlgorithm("Scale");
  scale->initialize();
  scale->setProperty("InputWorkspace", ws);
  scale->setProperty("OutputWorkspace", ws);
  scale->setProperty("Factor", factor);
  const std::string operation = isMultiply ? "Multiply" : "Add";
  scale->setProperty("Operation", operation);
  scale->execute();
}

MatrixWorkspace_sptr PolarizerEfficiency::workspaceForSpinConfig(WorkspaceGroup_sptr group,
                                                                 const std::vector<std::string> &spinConfigOrder,
                                                                 const std::string &spinConfig) {
  const auto wsIndex =
      std::find(spinConfigOrder.cbegin(), spinConfigOrder.cend(), spinConfig) - spinConfigOrder.cbegin();
  return std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(wsIndex));
}
} // namespace Mantid::Algorithms