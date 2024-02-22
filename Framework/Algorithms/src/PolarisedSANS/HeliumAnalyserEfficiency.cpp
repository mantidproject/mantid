// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarisedSANS/HeliumAnalyserEfficiency.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <vector>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(HeliumAnalyserEfficiency)

using namespace Kernel;
using namespace API;

namespace SpinConfigurations {
static const std::string UpUp = "11";
static const std::string UpDown = "10";
static const std::string DownUp = "01";
static const std::string DownDown = "00";
} // namespace SpinConfigurations

/// Empty default constructor algorithm() calls the constructor in the base class
HeliumAnalyserEfficiency::HeliumAnalyserEfficiency() : Algorithm() {}

void HeliumAnalyserEfficiency::init() {
  // Declare required input parameters for algorithm and do some validation here
  auto val = std::make_shared<CompositeValidator>();
  val->add<WorkspaceUnitValidator>("Wavelength");
  val->add<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, val));
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputTransmissionWorkspace", "T", Direction::Output));
  declareProperty(std::make_unique<WorkspaceProperty<>>("p_He", "p_He", Direction::Output));
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputTransmissionParaWorkspace", "T_para", Direction::Output));
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputTransmissionAntiWorkspace", "T_anti", Direction::Output));

  std::vector<std::string> initialSpinConfig{
      {SpinConfigurations::UpUp, SpinConfigurations::UpDown, SpinConfigurations::DownUp, SpinConfigurations::DownDown}};
  std::sort(initialSpinConfig.begin(), initialSpinConfig.end());
  std::vector<std::string> allowedSpinConfigs;
  allowedSpinConfigs.push_back(boost::algorithm::join(initialSpinConfig, ","));
  while (std::next_permutation(initialSpinConfig.begin(), initialSpinConfig.end())) {
    allowedSpinConfigs.push_back(boost::algorithm::join(initialSpinConfig, ","));
  }
  declareProperty(
      "SpinConfigurations",
      boost::algorithm::join(std::vector<std::string>{{SpinConfigurations::UpUp, SpinConfigurations::DownUp,
                                                       SpinConfigurations::DownDown, SpinConfigurations::UpDown}},
                             ","),
      std::make_shared<ListValidator<std::string>>(allowedSpinConfigs));
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty("T_E", 0.9, mustBePositive);
  declareProperty("pxd", 12.0, mustBePositive, "Gas pressure in bar multiplied by cell length in metres");
  declareProperty("StartLambda", 1.75, mustBePositive, "Lower boundary of wavelength range to use for fitting");
  declareProperty("EndLambda", 8.0, mustBePositive, "Upper boundary of wavelength range to use for fitting");
}

/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an error message
 */
std::map<std::string, std::string> HeliumAnalyserEfficiency::validateInputs() {
  std::map<std::string, std::string> errorList;
  const auto ws = AnalysisDataService::Instance().retrieve(getProperty("InputWorkspace"));
  if (!ws->isGroup()) {
    errorList["InputWorkspace"] = "The input workspace is not a group workspace";
  } else {
    const auto wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
    if (wsGroup->size() != 4) {
      errorList["InputWorkspace"] =
          "The input group workspace must have four periods corresponding to the four spin configurations.";
    }
  }
  return errorList;
}

/**
 * Explicitly calls validateInputs and throws runtime error in case
 * of issues in the input properties.
 */
void HeliumAnalyserEfficiency::validateGroupInput() {
  const auto results = validateInputs();
  for (const auto &result : results) {
    throw std::runtime_error("Issue in " + result.first + " property: " + result.second);
  }
}

bool HeliumAnalyserEfficiency::processGroups() {
  validateGroupInput();
  calculateAnalyserEfficiency();
  return true;
}

void HeliumAnalyserEfficiency::exec() { calculateAnalyserEfficiency(); }

void HeliumAnalyserEfficiency::calculateAnalyserEfficiency() {
  // First we extract the individual workspaces corresponding to each spin configuration from the group workspace
  const auto groupWorkspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getProperty("InputWorkspace"));
  std::string spinConfigurationInput = getProperty("SpinConfigurations");
  std::vector<std::string> spinConfigurations;
  boost::split(spinConfigurations, spinConfigurationInput, boost::is_any_of(","));

  const auto t11Ws = workspaceForSpinConfig(groupWorkspace, spinConfigurations, SpinConfigurations::UpUp);
  const auto t10Ws = workspaceForSpinConfig(groupWorkspace, spinConfigurations, SpinConfigurations::UpDown);
  const auto t01Ws = workspaceForSpinConfig(groupWorkspace, spinConfigurations, SpinConfigurations::DownUp);
  const auto t00Ws = workspaceForSpinConfig(groupWorkspace, spinConfigurations, SpinConfigurations::DownDown);

  // T_NSF = T11 + T00 (NSF = not spin flipped)
  auto plus = createChildAlgorithm("Plus");
  plus->initialize();
  plus->setProperty("LHSWorkspace", t11Ws);
  plus->setProperty("RHSWorkspace", t00Ws);
  plus->setProperty("OutputWorkspace", "tnsf");
  plus->executeAsChildAlg();
  MatrixWorkspace_sptr tnsfWs = plus->getProperty("OutputWorkspace");

  // T_SF = T01 + T10 (SF = spin flipped)
  plus->initialize();
  plus->setProperty("LHSWorkspace", t01Ws);
  plus->setProperty("RHSWorkspace", t10Ws);
  plus->setProperty("OutputWorkspace", "tsf");
  plus->executeAsChildAlg();
  MatrixWorkspace_sptr tsfWs = plus->getProperty("OutputWorkspace");

  // P = tanh(mu * phe) where P is the polarisation of an unpolarised incoming beam
  // after the analyser cell. We're going to calculate P from the data,
  // P = (T_NSF - T_SF) / (T_NSF + T_SF), then fit tanh(mu * phe) to it in order
  // to calculate phe.

  plus->initialize();
  plus->setProperty("LHSWorkspace", tnsfWs);
  plus->setProperty("RHSWorkspace", tsfWs);
  plus->setProperty("OutputWorkspace", "denominator");
  plus->executeAsChildAlg();
  MatrixWorkspace_sptr denom = plus->getProperty("OutputWorkspace");

  auto minus = createChildAlgorithm("Minus");
  minus->initialize();
  minus->setProperty("LHSWorkspace", tnsfWs);
  minus->setProperty("RHSWorkspace", tsfWs);
  minus->setProperty("OutputWorkspace", "numerator");
  minus->executeAsChildAlg();
  MatrixWorkspace_sptr numerator = minus->getProperty("OutputWorkspace");

  auto divide = createChildAlgorithm("Divide");
  divide->initialize();
  divide->setProperty("LHSWorkspace", numerator);
  divide->setProperty("RHSWorkspace", denom);
  divide->setProperty("OutputWorkspace", "p");
  divide->executeAsChildAlg();
  MatrixWorkspace_sptr p = divide->getProperty("OutputWorkspace");

  const double pxd = getProperty("pxd");
  const double mu = ABSORPTION_CROSS_SECTION_CONSTANT * pxd;

  auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", "name=UserFunction,Formula=tanh(" + std::to_string(mu) + "*phe*x),phe=0.1");
  fit->setProperty("InputWorkspace", p);
  const double startLambda = getProperty("StartLambda");
  fit->setProperty("StartX", startLambda);
  const double endLambda = getProperty("EndLambda");
  fit->setProperty("EndX", endLambda);
  fit->setProperty("CreateOutput", true);
  fit->executeAsChildAlg();
  ITableWorkspace_sptr fitParameters = fit->getProperty("OutputParameters");
  MatrixWorkspace_sptr fitWorkspace = fit->getProperty("OutputWorkspace");

  double pHe = fitParameters->getRef<double>("Value", 0);

  auto createSingleValuedWorkspace = createChildAlgorithm("CreateSingleValuedWorkspace");
  createSingleValuedWorkspace->initialize();
  createSingleValuedWorkspace->setProperty("DataValue", pHe);
  createSingleValuedWorkspace->setProperty("ErrorValue", fitParameters->getRef<double>("Error", 0));
  createSingleValuedWorkspace->setProperty("OutputWorkspace", "phe");
  createSingleValuedWorkspace->executeAsChildAlg();
  MatrixWorkspace_sptr pheWs = createSingleValuedWorkspace->getProperty("OutputWorkspace");

  setProperty("p_He", pheWs);
  const double t_E = getProperty("T_E");

  // Now we have all the parameters to calculate T(lambda), the transmission of the helium
  // analyser for an incident unpolarised beam. T_para and T_anti are also calculated, the
  // transmission of the wanted and unwanted spin state. T = T_para + T_anti

  const auto wavelengthValuesHist = fitWorkspace->x(0);
  std::vector<double> wavelengthValues(wavelengthValuesHist.cbegin(), wavelengthValuesHist.cend());
  std::vector<double> tPara(wavelengthValues.size());
  std::vector<double> tAnti(wavelengthValues.size());

  std::transform(wavelengthValues.cbegin(), wavelengthValues.cend(), tPara.begin(),
                 [pHe, t_E, mu](double w) { return 0.5 * t_E * std::exp(-mu * w * (1 - pHe)); });
  std::transform(wavelengthValues.cbegin(), wavelengthValues.cend(), tAnti.begin(),
                 [pHe, t_E, mu](double w) { return 0.5 * t_E * std::exp(-mu * w * (1 + pHe)); });

  auto createWorkspace = createChildAlgorithm("CreateWorkspace");
  createWorkspace->initialize();
  createWorkspace->setProperty("OutputWorkspace", "tPara");
  createWorkspace->setProperty("DataX", wavelengthValues);
  createWorkspace->setProperty("DataY", tPara);
  createWorkspace->setProperty("UnitX", "Wavelength");
  createWorkspace->setProperty("WorkspaceTitle", "Helium Analyser Transmission T_para");
  createWorkspace->executeAsChildAlg();
  MatrixWorkspace_sptr tParaWorkspace = createWorkspace->getProperty("OutputWorkspace");
  setProperty("OutputTransmissionParaWorkspace", tParaWorkspace);

  createWorkspace->initialize();
  createWorkspace->setProperty("OutputWorkspace", "tAnti");
  createWorkspace->setProperty("DataX", wavelengthValues);
  createWorkspace->setProperty("DataY", tAnti);
  createWorkspace->setProperty("UnitX", "Wavelength");
  createWorkspace->setProperty("WorkspaceTitle", "Helium Analyser Transmission T_anti");
  createWorkspace->executeAsChildAlg();
  MatrixWorkspace_sptr tAntiWorkspace = createWorkspace->getProperty("OutputWorkspace");
  setProperty("OutputTransmissionAntiWorkspace", tAntiWorkspace);

  plus->initialize();
  plus->setProperty("LHSWorkspace", tParaWorkspace);
  plus->setProperty("RHSWorkspace", tAntiWorkspace);
  plus->setProperty("OutputWorkspace", "T");
  plus->executeAsChildAlg();
  MatrixWorkspace_sptr transmissionWorkspace = plus->getProperty("OutputWorkspace");
  setProperty("OutputTransmissionWorkspace", transmissionWorkspace);

  // Errors?
}

Workspace_sptr HeliumAnalyserEfficiency::workspaceForSpinConfig(WorkspaceGroup_sptr group,
                                                                const std::vector<std::string> &spinConfigOrder,
                                                                const std::string &spinConfig) {
  const auto wsIndex =
      std::find(spinConfigOrder.cbegin(), spinConfigOrder.cend(), spinConfig) - spinConfigOrder.cbegin();
  return group->getItem(wsIndex);
}

const double HeliumAnalyserEfficiency::ABSORPTION_CROSS_SECTION_CONSTANT = 0.0733;
} // namespace Mantid::Algorithms