// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiency.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <boost/math/distributions/students_t.hpp>
#include <vector>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(HeliumAnalyserEfficiency)

using namespace Kernel;
using namespace API;

const double HeliumAnalyserEfficiency::ABSORPTION_CROSS_SECTION_CONSTANT = 0.0733;

namespace PropertyNames {
static const std::string INPUT_WORKSPACE = "InputWorkspace";
static const std::string OUTPUT_WORKSPACE = "OutputWorkspace";
static const std::string SPIN_STATES = "SpinStates";
static const std::string PD = "GasPressureTimesCellLength";
static const std::string PD_ERROR = "GasPressureTimesCellLengthError";
static const std::string START_LAMBDA = "StartLambda";
static const std::string END_LAMBDA = "EndLambda";
static const std::string IGNORE_FIT_QUALITY_ERROR = "IgnoreFitQualityError";
static const std::string GROUP_INPUTS = "Inputs";
static const std::string GROUP_FIT_OPTIONS = "Fit Options";
} // namespace PropertyNames

void HeliumAnalyserEfficiency::init() {
  // Declare required input parameters for algorithm and do some validation here
  auto validator = std::make_shared<CompositeValidator>();
  validator->add<WorkspaceUnitValidator>("Wavelength");
  validator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input, validator),
      "Input group workspace to use for polarization calculation");
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
                  "Helium analyzer efficiency as a function of wavelength");

  auto spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  std::string initialSpinConfig = "11,10,01,00";
  declareProperty(PropertyNames::SPIN_STATES, initialSpinConfig, spinValidator,
                  "Order of individual spin states in the input group workspace, e.g. \"01,11,00,10\"");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::PD, 12.0, mustBePositive, "Gas pressure in bar multiplied by cell length in metres");
  declareProperty(PropertyNames::PD_ERROR, 0.0, mustBePositive, "Error in gas pressure multiplied by cell length");
  declareProperty(PropertyNames::START_LAMBDA, 1.75, mustBePositive,
                  "Lower boundary of wavelength range to use for fitting");
  declareProperty(PropertyNames::END_LAMBDA, 8.0, mustBePositive,
                  "Upper boundary of wavelength range to use for fitting");
  declareProperty(PropertyNames::IGNORE_FIT_QUALITY_ERROR, false,
                  "Whether the algorithm should ignore a poor chi-squared (fit cost value) of greater than 1 and "
                  "therefore not throw an error",
                  Direction::Input);

  setPropertyGroup(PropertyNames::SPIN_STATES, PropertyNames::GROUP_INPUTS);
  setPropertyGroup(PropertyNames::PD, PropertyNames::GROUP_INPUTS);
  setPropertyGroup(PropertyNames::PD_ERROR, PropertyNames::GROUP_INPUTS);

  setPropertyGroup(PropertyNames::START_LAMBDA, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::END_LAMBDA, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::IGNORE_FIT_QUALITY_ERROR, PropertyNames::GROUP_FIT_OPTIONS);
}

/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an error message
 */
std::map<std::string, std::string> HeliumAnalyserEfficiency::validateInputs() {
  std::map<std::string, std::string> errorList;
  const std::string inputWorkspaceName = getProperty(PropertyNames::INPUT_WORKSPACE);
  if (!AnalysisDataService::Instance().doesExist(inputWorkspaceName)) {
    errorList[PropertyNames::INPUT_WORKSPACE] =
        "The input workspace " + inputWorkspaceName + " does not exist in the ADS.";
  } else {
    const auto ws = AnalysisDataService::Instance().retrieve(getProperty(PropertyNames::INPUT_WORKSPACE));
    if (!ws->isGroup()) {
      errorList[PropertyNames::INPUT_WORKSPACE] = "The input workspace is not a group workspace";
    } else {
      const auto wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
      if (wsGroup->size() != 4) {
        errorList[PropertyNames::INPUT_WORKSPACE] =
            "The input group workspace must have four periods corresponding to the four spin configurations.";
      }
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
  const auto groupWorkspace =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getProperty(PropertyNames::INPUT_WORKSPACE));
  const std::string spinConfigurationInput = getProperty(PropertyNames::SPIN_STATES);

  const auto t11Ws = PolarizationCorrectionsHelpers::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           SpinStateValidator::ONE_ONE);
  const auto t10Ws = PolarizationCorrectionsHelpers::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           SpinStateValidator::ONE_ZERO);
  const auto t01Ws = PolarizationCorrectionsHelpers::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           SpinStateValidator::ZERO_ONE);
  const auto t00Ws = PolarizationCorrectionsHelpers::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           SpinStateValidator::ZERO_ZERO);

  // T_NSF = T11 + T00 (NSF = not spin flipped)
  MatrixWorkspace_sptr tnsfWs = addTwoWorkspaces(t11Ws, t00Ws);

  // T_SF = T01 + T10 (SF = spin flipped)
  MatrixWorkspace_sptr tsfWs = addTwoWorkspaces(t01Ws, t10Ws);

  // e = (1 + tanh(mu * phe))/2 where e is the efficiency of the analyser.
  // We're going to calculate e from the data, e = T_NSF / (T_NSF + T_SF),
  // then fit (1 + tanh(mu * phe))/2 to it in order to calculate phe, the
  // helium atom polarization in the analyser.
  MatrixWorkspace_sptr denom = addTwoWorkspaces(tnsfWs, tsfWs);
  MatrixWorkspace_sptr e = divideWorkspace(tnsfWs, denom);

  // Now we fit (1 + tanh(mu*pHe*x))/2 to P to give us pHe

  const double pd = getProperty(PropertyNames::PD);
  const double mu = ABSORPTION_CROSS_SECTION_CONSTANT * pd;

  const MantidVec wavelengthValues = e->dataX(0);
  double pHe, pHeError;
  MantidVec eCalc;
  fitAnalyserEfficiency(mu, e, wavelengthValues, pHe, pHeError, eCalc);
  auto efficiency = calculateEfficiencyWorkspace(wavelengthValues, eCalc, pHe, pHeError, mu, pd);
  setProperty(PropertyNames::OUTPUT_WORKSPACE, efficiency);
}

void HeliumAnalyserEfficiency::fitAnalyserEfficiency(const double mu, MatrixWorkspace_sptr e,
                                                     const MantidVec &wavelengthValues, double &pHe, double &pHeError,
                                                     MantidVec &eCalc) {
  auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", "name=UserFunction,Formula=(1 + tanh(" + std::to_string(mu) + "*phe*x))/2,phe=0.1");
  fit->setProperty("InputWorkspace", e);
  const double startLambda = getProperty(PropertyNames::START_LAMBDA);
  fit->setProperty("StartX", startLambda);
  const double endLambda = getProperty(PropertyNames::END_LAMBDA);
  fit->setProperty("EndX", endLambda);
  fit->setProperty("CreateOutput", true);
  fit->execute();

  const bool ignoreFitQualityError = getProperty(PropertyNames::IGNORE_FIT_QUALITY_ERROR);
  const std::string &status = fit->getProperty("OutputStatus");
  if (!ignoreFitQualityError && (!fit->isExecuted() || status != "success")) {
    auto const &errMsg{"Failed to fit to data in the calculation of p_He: " + status};
    g_log.error(errMsg);
    throw std::runtime_error(errMsg);
  }

  ITableWorkspace_sptr fitParameters = fit->getProperty("OutputParameters");
  MatrixWorkspace_sptr fitWorkspace = fit->getProperty("OutputWorkspace");

  pHe = fitParameters->getRef<double>("Value", 0);
  pHeError = fitParameters->getRef<double>("Error", 0);
  eCalc = MantidVec(wavelengthValues.size());
  for (size_t i = 0; i < eCalc.size(); ++i) {
    eCalc[i] = (1 + std::tanh(mu * pHe * wavelengthValues[i])) / 2.0;
  }
}

MatrixWorkspace_sptr HeliumAnalyserEfficiency::calculateEfficiencyWorkspace(const MantidVec &wavelengthValues,
                                                                            const MantidVec &eValues, const double pHe,
                                                                            const double pHeError, const double mu,
                                                                            const double pd) {
  // This value is used to give us the correct error bounds
  const double tCrit = calculateTCrit(wavelengthValues.size());
  const double pdError = getProperty(PropertyNames::PD_ERROR);

  // This is the error calculation for the efficiency using the error on pHe and
  // the supplied covariance matrix (if there is one).

  auto efficiencyErrors = MantidVec(eValues.size());

  for (size_t i = 0; i < eValues.size(); ++i) {
    const double w = wavelengthValues[i];
    const auto commonTerm = 0.5 * w / std::pow(std::cosh(mu * w * pHe), 2);
    const double de_dpHe = mu * commonTerm;
    const double de_dpd = ABSORPTION_CROSS_SECTION_CONSTANT * pHe * commonTerm;
    // Covariance between p_He and pd is zero
    efficiencyErrors[i] =
        tCrit * std::sqrt(de_dpHe * de_dpHe * pHeError * pHeError + de_dpd * de_dpd * pdError * pdError);
  }

  return createWorkspace(getPropertyValue(PropertyNames::OUTPUT_WORKSPACE), "Analyser Efficiency", wavelengthValues,
                         eValues, efficiencyErrors);
}

double HeliumAnalyserEfficiency::calculateTCrit(const size_t numberOfBins) {
  // Create a t distribution with dof given by the number of data points minus
  // the number of params (3)
  double tPpf = 1;
  if (numberOfBins > 3) {
    const boost::math::students_t dist(static_cast<double>(numberOfBins) - 3.0);
    // Critical value corresponding to 1-sigma
    const double alpha = (1 + std::erf(1.0 / sqrt(2))) / 2;
    // Scale factor for the error calculations
    tPpf = boost::math::quantile(dist, alpha);
  } else {
    g_log.warning(
        "The number of histogram bins must be greater than 3 in order to provide an accurate error calculation");
  }
  return tPpf;
}

MatrixWorkspace_sptr HeliumAnalyserEfficiency::addTwoWorkspaces(MatrixWorkspace_sptr ws, MatrixWorkspace_sptr otherWs) {
  auto plus = createChildAlgorithm("Plus");
  plus->initialize();
  plus->setProperty("LHSWorkspace", ws);
  plus->setProperty("RHSWorkspace", otherWs);
  plus->execute();
  return plus->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr HeliumAnalyserEfficiency::createWorkspace(const std::string &name, const std::string &title,
                                                               const MantidVec &xData, const MantidVec &yData,
                                                               const MantidVec &eData, const bool addToAds) {
  auto createWorkspace = createChildAlgorithm("CreateWorkspace");
  createWorkspace->initialize();
  createWorkspace->setProperty("OutputWorkspace", name);
  createWorkspace->setProperty("DataX", xData);
  createWorkspace->setProperty("DataY", yData);
  createWorkspace->setProperty("DataE", eData);
  createWorkspace->setProperty("UnitX", "Wavelength");
  createWorkspace->setProperty("WorkspaceTitle", title);
  createWorkspace->execute();
  MatrixWorkspace_sptr ws = createWorkspace->getProperty("OutputWorkspace");
  if (addToAds) {
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }
  return ws;
}

MatrixWorkspace_sptr HeliumAnalyserEfficiency::divideWorkspace(MatrixWorkspace_sptr numerator,
                                                               MatrixWorkspace_sptr denominator) {
  auto divide = createChildAlgorithm("Divide");
  divide->initialize();
  divide->setProperty("LHSWorkspace", numerator);
  divide->setProperty("RHSWorkspace", denominator);
  divide->setProperty("OutputWorkspace", "p");
  divide->execute();
  return divide->getProperty("OutputWorkspace");
}
} // namespace Mantid::Algorithms