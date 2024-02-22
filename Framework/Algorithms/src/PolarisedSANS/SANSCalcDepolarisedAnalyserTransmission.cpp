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
double constexpr START_X = 1.75;
double constexpr END_X = 14;
std::string_view constexpr FIT_SUCCESS{"success"};

std::string createFunctionStr() {
  std::ostringstream func;
  func << "name=UserFunction, Formula=" << T_E_NAME << "*exp(" << LAMBDA_CONVERSION_FACTOR << "*" << PXD_NAME << "*x)";
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
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(Prop::DEP_WORKSPACE), "",
                                                                       Kernel::Direction::Input),
                  "The group of fully depolarised workspaces.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(Prop::MT_WORKSPACE), "",
                                                                       Kernel::Direction::Input),
                  "The group of empty cell workspaces.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(std::string(Prop::OUTPUT_WORKSPACE), "",
                                                                       Kernel::Direction::Output),
                  "The name of the output table workspace containing the fit parameter results.");
}

void SANSCalcDepolarisedAnalyserTransmission::exec() {
  auto const &dividedWs = calcDepolarisedProportion();
  auto const &fitParameterWs =
      calcWavelengthDependentTransmission(dividedWs, getPropertyValue(std::string(Prop::OUTPUT_WORKSPACE)));
  setProperty(std::string(Prop::OUTPUT_WORKSPACE), fitParameterWs);
}

MatrixWorkspace_sptr SANSCalcDepolarisedAnalyserTransmission::calcDepolarisedProportion() {
  auto const &depWsName = getPropertyValue(std::string(Prop::DEP_WORKSPACE));
  auto const &mtWsName = getPropertyValue(std::string(Prop::MT_WORKSPACE));
  auto divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", depWsName);
  divideAlg->setProperty("RHSWorkspace", mtWsName);
  divideAlg->execute();
  return divideAlg->getProperty(std::string(Prop::OUTPUT_WORKSPACE));
}

ITableWorkspace_sptr
SANSCalcDepolarisedAnalyserTransmission::calcWavelengthDependentTransmission(MatrixWorkspace_sptr const &inputWs,
                                                                             std::string const &outputWsName) {
  auto const &func = FunctionFactory::Instance().createInitialized(FitValues::createFunctionStr());
  auto fitAlg = createChildAlgorithm("Fit");
  fitAlg->setProperty("Function", func);
  fitAlg->setProperty("InputWorkspace", inputWs);
  fitAlg->setProperty("IgnoreInvalidData", true);
  fitAlg->setProperty("StartX", FitValues::START_X);
  fitAlg->setProperty("EndX", FitValues::END_X);
  fitAlg->setProperty("OutputParametersOnly", true);
  fitAlg->setPropertyValue("Output", outputWsName);
  fitAlg->execute();

  std::string const &status = fitAlg->getProperty("OutputStatus");
  if (!fitAlg->isExecuted() || status != FitValues::FIT_SUCCESS) {
    auto const &errMsg{"Failed to fit to divided workspace, " + inputWs->getName() + ": " + status};
    throw std::runtime_error(errMsg);
  }

  return fitAlg->getProperty("OutputParameters");
}

} // namespace Mantid::Algorithms
