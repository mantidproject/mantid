// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarizationCorrections/DepolarizedAnalyserTransmission.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/Divide.h"

namespace {
/// Property Names
namespace PropNames {
std::string_view constexpr DEP_WORKSPACE{"DepolarisedWorkspace"};
std::string_view constexpr MT_WORKSPACE{"EmptyCellWorkspace"};
std::string_view constexpr EMPTY_CELL_TRANS_START{"TEStartingValue"};
std::string_view constexpr DEPOL_OPACITY_START{"PxDStartingValue"};
std::string_view constexpr OUTPUT_WORKSPACE{"OutputWorkspace"};
} // namespace PropNames

/// Initial fitting function values.
namespace FitValues {
double constexpr LAMBDA_CONVERSION_FACTOR = -0.0733;
double constexpr EMPTY_CELL_TRANS_START = 0.9;
double constexpr DEPOL_OPACITY_START = 12.6;
std::string_view constexpr EMPTY_CELL_TRANS_NAME = "T_E";
std::string_view constexpr DEPOL_OPACITY_NAME = "pxd";
double constexpr START_X = 1.75;
double constexpr END_X = 14;
std::string_view constexpr FIT_SUCCESS{"success"};

std::ostringstream createFunctionStrStream() {
  std::ostringstream func;
  func << "name=UserFunction, Formula=" << EMPTY_CELL_TRANS_NAME << "*exp(" << LAMBDA_CONVERSION_FACTOR << "*"
       << DEPOL_OPACITY_NAME << "*x)";
  return func;
}
} // namespace FitValues
} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(DepolarizedAnalyserTransmission)

std::string const DepolarizedAnalyserTransmission::summary() const {
  return "Calculate the transmission rate through a depolarised He3 cell.";
}

void DepolarizedAnalyserTransmission::init() {
  auto wsValidator = std::make_shared<WorkspaceUnitValidator>("Wavelength");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(PropNames::DEP_WORKSPACE), "",
                                                           Kernel::Direction::Input, wsValidator),
      "The fully depolarised helium cell workspace. Should contain a single spectra. Units must be in wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(PropNames::MT_WORKSPACE), "",
                                                                       Kernel::Direction::Input, wsValidator),
                  "The empty cell workspace. Must contain a single spectra. Units must be in wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(std::string(PropNames::OUTPUT_WORKSPACE), "",
                                                                       Kernel::Direction::Output),
                  "The name of the output table workspace containing the fit parameter results.");
  declareProperty(std::string(PropNames::EMPTY_CELL_TRANS_START), FitValues::EMPTY_CELL_TRANS_START,
                  "Starting value for the empty analyser cell transmission fit property " +
                      std::string(FitValues::EMPTY_CELL_TRANS_NAME) + ".");
  declareProperty(std::string(PropNames::DEPOL_OPACITY_START), FitValues::DEPOL_OPACITY_START,
                  "Starting value for the depolarised cell transmission fit property " +
                      std::string(FitValues::DEPOL_OPACITY_NAME) + ".");
}

std::map<std::string, std::string> DepolarizedAnalyserTransmission::validateInputs() {
  std::map<std::string, std::string> result;
  MatrixWorkspace_sptr const &depWs = getProperty(std::string(PropNames::DEP_WORKSPACE));
  if (depWs->getNumberHistograms() != 1) {
    result[std::string(PropNames::DEP_WORKSPACE)] =
        "The depolarised workspace must contain a single spectrum. Contains " +
        std::to_string(depWs->getNumberHistograms()) + " spectra.";
  }
  MatrixWorkspace_sptr const &mtWs = getProperty(std::string(PropNames::MT_WORKSPACE));
  if (mtWs->getNumberHistograms() != 1) {
    result[std::string(PropNames::MT_WORKSPACE)] =
        "The empty cell workspace must contain a single spectrum. Contains " +
        std::to_string(mtWs->getNumberHistograms()) + " spectra.";
  }
  return result;
}

void DepolarizedAnalyserTransmission::exec() {
  auto const &dividedWs = calcDepolarisedProportion();
  ITableWorkspace_sptr const &fitParameterWs =
      calcWavelengthDependentTransmission(dividedWs, getPropertyValue(std::string(PropNames::OUTPUT_WORKSPACE)));
  setProperty(std::string(PropNames::OUTPUT_WORKSPACE), fitParameterWs);
}

MatrixWorkspace_sptr DepolarizedAnalyserTransmission::calcDepolarisedProportion() {
  MatrixWorkspace_sptr const &depWs = getProperty(std::string(PropNames::DEP_WORKSPACE));
  MatrixWorkspace_sptr const &mtWs = getProperty(std::string(PropNames::MT_WORKSPACE));
  auto divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", depWs);
  divideAlg->setProperty("RHSWorkspace", mtWs);
  divideAlg->execute();
  return divideAlg->getProperty(std::string(PropNames::OUTPUT_WORKSPACE));
}

ITableWorkspace_sptr
DepolarizedAnalyserTransmission::calcWavelengthDependentTransmission(MatrixWorkspace_sptr const &inputWs,
                                                                     std::string const &outputWsName) {
  auto funcStream = FitValues::createFunctionStrStream();
  funcStream << "," << FitValues::EMPTY_CELL_TRANS_NAME << "="
             << getPropertyValue(std::string(PropNames::EMPTY_CELL_TRANS_START));
  funcStream << "," << FitValues::DEPOL_OPACITY_NAME << "="
             << getPropertyValue(std::string(PropNames::DEPOL_OPACITY_START));
  auto const &func = FunctionFactory::Instance().createInitialized(funcStream.str());
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
    auto const &errMsg{"Failed to fit to transmission workspace, " + inputWs->getName() + ": " + status};
    throw std::runtime_error(errMsg);
  }
  // If a non-monitor MT workspace is provided by mistake the workspace to be fitted can contain only NaNs/infs due to
  // divide-by-0 results. In this case, the fit succeeds but the quality is 0, so we should still throw an error.
  double const &fitQuality = fitAlg->getProperty("OutputChi2overDoF");
  if (fitQuality <= 0) {
    throw std::runtime_error("Failed to fit to transmission workspace, " + inputWs->getName() +
                             ": Fit quality is too low (" + std::to_string(fitQuality) +
                             "). You may want to check that the correct monitor spectrum was provided.");
  }
  return fitAlg->getProperty("OutputParameters");
}

} // namespace Mantid::Algorithms
