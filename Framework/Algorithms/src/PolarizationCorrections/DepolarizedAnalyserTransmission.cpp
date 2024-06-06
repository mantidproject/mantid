// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PolarizationCorrections/DepolarizedAnalyserTransmission.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/CompositeValidator.h"

namespace {
/// Property Names
namespace PropNames {
std::string_view constexpr DEP_WORKSPACE{"DepolarizedWorkspace"};
std::string_view constexpr MT_WORKSPACE{"EmptyCellWorkspace"};
std::string_view constexpr EMPTY_CELL_TRANS_START{"TEStartingValue"};
std::string_view constexpr DEPOL_OPACITY_START{"PxDStartingValue"};
std::string_view constexpr START_X = "StartX";
std::string_view constexpr END_X = "EndX";
std::string_view constexpr IGNORE_FIT_QUALITY{"IgnoreFitQualityError"};
std::string_view constexpr OUTPUT_WORKSPACE{"OutputWorkspace"};
std::string_view constexpr OUTPUT_FIT{"OutputFitCurves"};
std::string_view constexpr GROUP_INPUT{"Input Workspaces"};
std::string_view constexpr GROUP_OUTPUT{"Output Workspaces"};
std::string_view constexpr GROUP_FIT{"Fit Starting Values"};
} // namespace PropNames

/// Initial fitting function values.
namespace FitValues {
using namespace Mantid::API;

double constexpr LAMBDA_CONVERSION_FACTOR = -0.0733;
double constexpr EMPTY_CELL_TRANS_START = 0.9;
double constexpr DEPOL_OPACITY_START = 12.6;
std::string_view constexpr EMPTY_CELL_TRANS_NAME = "T_E";
std::string_view constexpr DEPOL_OPACITY_NAME = "pxd";
double constexpr START_X_START = 1.75;
double constexpr END_X_START = 14;
std::string_view constexpr FIT_SUCCESS{"success"};

std::shared_ptr<IFunction> createFunction(std::string const &mtTransStart, std::string const &depolOpacStart) {
  std::ostringstream funcSS;
  funcSS << "name=UserFunction, Formula=" << EMPTY_CELL_TRANS_NAME << "*exp(" << LAMBDA_CONVERSION_FACTOR << "*"
         << DEPOL_OPACITY_NAME << "*x)";
  funcSS << "," << EMPTY_CELL_TRANS_NAME << "=" << mtTransStart;
  funcSS << "," << DEPOL_OPACITY_NAME << "=" << depolOpacStart;
  return FunctionFactory::Instance().createInitialized(funcSS.str());
}
} // namespace FitValues

inline void validateWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace, std::string const &prop,
                              std::map<std::string, std::string> &result) {
  if (workspace->getNumberHistograms() != 1) {
    result[prop] = prop + " must contain a single spectrum. Contains " +
                   std::to_string(workspace->getNumberHistograms()) + " spectra.";
  }
}

} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm in the AlgorithmFactory
DECLARE_ALGORITHM(DepolarizedAnalyserTransmission)

std::string const DepolarizedAnalyserTransmission::summary() const {
  return "Calculate the transmission rate through a depolarized He3 cell.";
}

void DepolarizedAnalyserTransmission::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(PropNames::DEP_WORKSPACE), "",
                                                           Kernel::Direction::Input, wsValidator),
      "The fully depolarized helium cell workspace. Should contain a single spectra. Units must be in wavelength.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(PropNames::MT_WORKSPACE), "",
                                                                       Kernel::Direction::Input, wsValidator),
                  "The empty cell workspace. Must contain a single spectra. Units must be in wavelength");
  declareProperty(std::string(PropNames::EMPTY_CELL_TRANS_START), FitValues::EMPTY_CELL_TRANS_START,
                  "Starting value for the empty analyser cell transmission fit property " +
                      std::string(FitValues::EMPTY_CELL_TRANS_NAME) + ".");
  declareProperty(std::string(PropNames::DEPOL_OPACITY_START), FitValues::DEPOL_OPACITY_START,
                  "Starting value for the depolarized cell transmission fit property " +
                      std::string(FitValues::DEPOL_OPACITY_NAME) + ".");
  declareProperty(std::string(PropNames::START_X), FitValues::START_X_START, "StartX value for the fit.");
  declareProperty(std::string(PropNames::END_X), FitValues::END_X_START, "EndX value for the fit.");
  declareProperty(std::string(PropNames::IGNORE_FIT_QUALITY), false,
                  "Whether the algorithm should ignore a poor chi-squared (fit cost value) of greater than 1 and "
                  "therefore not throw an error.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(std::string(PropNames::OUTPUT_WORKSPACE), "",
                                                                       Kernel::Direction::Output),
                  "The name of the table workspace containing the fit parameter results.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      std::string(PropNames::OUTPUT_FIT), "", Kernel::Direction::Output, PropertyMode::Optional),
                  "The name of the workspace containing the calculated fit curve.");

  auto const &inputGroup = std::string(PropNames::GROUP_INPUT);
  setPropertyGroup(std::string(PropNames::DEP_WORKSPACE), inputGroup);
  setPropertyGroup(std::string(PropNames::MT_WORKSPACE), inputGroup);
  auto const &fitGroup = std::string(PropNames::GROUP_FIT);
  setPropertyGroup(std::string(PropNames::EMPTY_CELL_TRANS_START), fitGroup);
  setPropertyGroup(std::string(PropNames::DEPOL_OPACITY_START), fitGroup);
  auto const &outputGroup = std::string(PropNames::GROUP_OUTPUT);
  setPropertyGroup(std::string(PropNames::OUTPUT_WORKSPACE), outputGroup);
  setPropertyGroup(std::string(PropNames::OUTPUT_FIT), outputGroup);
}

std::map<std::string, std::string> DepolarizedAnalyserTransmission::validateInputs() {
  std::map<std::string, std::string> result;
  MatrixWorkspace_sptr const &depWs = getProperty(std::string(PropNames::DEP_WORKSPACE));
  if (depWs == nullptr) {
    result[std::string(PropNames::DEP_WORKSPACE)] =
        std::string(PropNames::DEP_WORKSPACE) + " must be a MatrixWorkspace.";
    return result;
  }
  validateWorkspace(depWs, std::string(PropNames::DEP_WORKSPACE), result);

  MatrixWorkspace_sptr const &mtWs = getProperty(std::string(PropNames::MT_WORKSPACE));
  if (mtWs == nullptr) {
    result[std::string(PropNames::MT_WORKSPACE)] = std::string(PropNames::MT_WORKSPACE) + " must be a MatrixWorkspace.";
    return result;
  }
  validateWorkspace(mtWs, std::string(PropNames::MT_WORKSPACE), result);

  if (!WorkspaceHelpers::matchingBins(*depWs, *mtWs, true)) {
    result[std::string(PropNames::DEP_WORKSPACE)] = "The bins in the " + std::string(PropNames::DEP_WORKSPACE) +
                                                    " and " + std::string(PropNames::MT_WORKSPACE) + " do not match.";
  }
  return result;
}

void DepolarizedAnalyserTransmission::exec() {
  auto const &dividedWs = calcDepolarizedProportion();
  calcWavelengthDependentTransmission(dividedWs, getPropertyValue(std::string(PropNames::OUTPUT_WORKSPACE)));
}

MatrixWorkspace_sptr DepolarizedAnalyserTransmission::calcDepolarizedProportion() {
  MatrixWorkspace_sptr const &depWs = getProperty(std::string(PropNames::DEP_WORKSPACE));
  MatrixWorkspace_sptr const &mtWs = getProperty(std::string(PropNames::MT_WORKSPACE));
  auto divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", depWs);
  divideAlg->setProperty("RHSWorkspace", mtWs);
  divideAlg->execute();
  return divideAlg->getProperty(std::string(PropNames::OUTPUT_WORKSPACE));
}

void DepolarizedAnalyserTransmission::calcWavelengthDependentTransmission(MatrixWorkspace_sptr const &inputWs,
                                                                          std::string const &outputWsName) {
  auto func = FitValues::createFunction(getPropertyValue(std::string(PropNames::EMPTY_CELL_TRANS_START)),
                                        getPropertyValue(std::string(PropNames::DEPOL_OPACITY_START)));
  auto fitAlg = createChildAlgorithm("Fit");
  double const &startX = getProperty(std::string(PropNames::START_X));
  double const &endX = getProperty(std::string(PropNames::END_X));
  fitAlg->setProperty("Function", func);
  fitAlg->setProperty("InputWorkspace", inputWs);
  fitAlg->setProperty("IgnoreInvalidData", true);
  fitAlg->setProperty("StartX", startX);
  fitAlg->setProperty("EndX", endX);
  fitAlg->setPropertyValue("Output", outputWsName);
  fitAlg->execute();

  std::string const &status = fitAlg->getProperty("OutputStatus");
  if (!fitAlg->isExecuted() || status != FitValues::FIT_SUCCESS) {
    auto const &errMsg{"Failed to fit to transmission workspace, " + inputWs->getName() + ": " + status};
    throw std::runtime_error(errMsg);
  }
  double const &fitQuality = fitAlg->getProperty("OutputChi2overDoF");
  bool const &qualityOverride = getProperty(std::string(PropNames::IGNORE_FIT_QUALITY));
  if (fitQuality == 0 || (fitQuality > 1 && !qualityOverride)) {
    throw std::runtime_error("Failed to fit to transmission workspace, " + inputWs->getName() +
                             ": Fit quality (chi-squared) is too poor (" + std::to_string(fitQuality) +
                             ". Should be 0 < x < 1). You may want to check that the correct spectrum and starting "
                             "fitting values were provided.");
  }
  ITableWorkspace_sptr const &paramWs = fitAlg->getProperty("OutputParameters");
  setProperty(std::string(PropNames::OUTPUT_WORKSPACE), paramWs);

  if (!getPropertyValue(std::string(PropNames::OUTPUT_FIT)).empty()) {
    MatrixWorkspace_sptr const &fitWs = fitAlg->getProperty("OutputWorkspace");
    setProperty(std::string(PropNames::OUTPUT_FIT), fitWs);
  }
}

} // namespace Mantid::Algorithms
