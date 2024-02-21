// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarisedSANS/SANSCalcDepolarisedAnalyserTransmission.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/Divide.h"

namespace {
/// Property Names
namespace Prop {
std::string_view constexpr DEP_WORKSPACE{"DepolarisedWorkspace"};
std::string_view constexpr MT_WORKSPACE{"EmptyCellWorkspace"};
std::string_view constexpr T_E_START{"T_EStartingValue"};
std::string_view constexpr PXD_START{"PxDStartingValue"};
std::string_view constexpr OUTPUT_WORKSPACE{"OutputWorkspace"};
} // namespace Prop

/// Initial fitting function values.
namespace FitValues {
double constexpr LAMBDA_CONVERSION_FACTOR = 0.0733;
double constexpr T_E_START = 0.9;
double constexpr PXD_START = 12.6;
std::string_view constexpr T_E_NAME = "t_e";
std::string_view constexpr PXD_NAME = "pxd";
double constexpr STARTX = 1.75;
double constexpr ENDX = 14;
std::string_view constexpr FIT_SUCCESS{"success"};

std::string createFunctionStr() {
  std::ostringstream func;
  func << "name=UserFunction, Formula=" << T_E_NAME << "exp(" << LAMBDA_CONVERSION_FACTOR << "*" << PXD_NAME << "*x)";
  return func.str();
}
} // namespace FitValues
} // namespace

namespace Mantid::Algorithms {

using namespace API;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(SANSCalcDepolarisedAnalyserTransmission)

std::string const SANSCalcDepolarisedAnalyserTransmission::summary() const {
  return "Calculate the transmission rate through a depolarised He3 cell.";
}

void SANSCalcDepolarisedAnalyserTransmission::init() {
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(std::string(Prop::DEP_WORKSPACE), "",
                                                                      Kernel::Direction::Input),
                  "The group of fully depolarised workspaces.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(std::string(Prop::MT_WORKSPACE), "",
                                                                      Kernel::Direction::Input),
                  "The group of empty cell workspaces.");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(std::string(Prop::OUTPUT_WORKSPACE), "",
                                                                      Kernel::Direction::Output),
                  "The name of the output workspace.");
}

void SANSCalcDepolarisedAnalyserTransmission::exec() {
  auto const &dividedWs = calcDepolarisedProportion();
  auto const &outputWsName = getPropertyValue(std::string(Prop::OUTPUT_WORKSPACE)) + "_Parameters";
  calcWavelengthDependentTransmission(dividedWs, getPropertyValue(std::string(Prop::OUTPUT_WORKSPACE)));
  setProperty(std::string(Prop::OUTPUT_WORKSPACE),
              AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputWsName));
}

std::string SANSCalcDepolarisedAnalyserTransmission::calcDepolarisedProportion() {
  auto const &depWsName = getPropertyValue(std::string(Prop::DEP_WORKSPACE));
  auto const &mtWsName = getPropertyValue(std::string(Prop::MT_WORKSPACE));
  auto const &outWsName = "__" + depWsName + mtWsName + "_div";
  auto divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", depWsName);
  divideAlg->setProperty("RHSWorkspace", mtWsName);
  divideAlg->setProperty("OutputWorkspace", outWsName);
  divideAlg->execute();
  return outWsName;
}

void SANSCalcDepolarisedAnalyserTransmission::calcWavelengthDependentTransmission(std::string const &inputWsName,
                                                                                  std::string const &outputWsName) {
  auto const &func = FunctionFactory::Instance().createInitialized(FitValues::createFunctionStr());
  auto fitAlg = createChildAlgorithm("Fit");
  fitAlg->setProperty("Function", func);
  fitAlg->setPropertyValue("InputWorkspace", inputWsName);
  fitAlg->setProperty("IgnoreInvalidData", true);
  fitAlg->setProperty("StartX", FitValues::STARTX);
  fitAlg->setProperty("EndX", FitValues::ENDX);
  fitAlg->setProperty("OutputParametersOnly", true);
  fitAlg->setPropertyValue("Output", outputWsName);
  fitAlg->execute();

  std::string const &status = fitAlg->getProperty("OutputStatus");
  if (!fitAlg->isExecuted() || status != FitValues::FIT_SUCCESS) {
    auto const &errMsg{"Failed to fit to divided workspace, " + inputWsName + ": " + status};
    throw std::runtime_error(errMsg);
  }
}

} // namespace Mantid::Algorithms
