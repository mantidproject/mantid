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

namespace PropertyNames {
static const std::string INPUT_WORKSPACE = "InputWorkspace";
static const std::string OUTPUT_WORKSPACE = "OutputWorkspace";
static const std::string P_HE = "HeliumPolarisation";
static const std::string OUTPUT_T_PARA_WORKSPACE = "OutputTransmissionParaWorkspace";
static const std::string OUTPUT_T_ANTI_WORKSPACE = "OutputTransmissionAntiWorkspace";
static const std::string SPIN_STATES = "SpinStates";
static const std::string T_E = "TransmissionEmptyCell";
static const std::string PXD = "GasPressureTimesCellLength";
static const std::string COVARIANCE = "Covariance";
static const std::string START_LAMBDA = "StartLambda";
static const std::string END_LAMBDA = "EndLambda";
static const std::string STOP_ON_FIT_ERROR = "StopOnFitError";
} // namespace PropertyNames

void HeliumAnalyserEfficiency::init() {
  // Declare required input parameters for algorithm and do some validation here
  auto validator = std::make_shared<CompositeValidator>();
  validator->add<WorkspaceUnitValidator>("Wavelength");
  validator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input, validator));
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WORKSPACE, "T", Direction::Output));
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::P_HE, "p_He", Direction::Output));
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_T_PARA_WORKSPACE, "T_para", Direction::Output));
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_T_ANTI_WORKSPACE, "T_anti", Direction::Output));

  auto spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  std::string initialSpinConfig = "11,10,01,00";
  declareProperty(PropertyNames::SPIN_STATES, initialSpinConfig, spinValidator);

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::T_E, 0.9, mustBePositive, "Transmission of the empty cell");
  declareProperty(PropertyNames::PXD, 12.0, mustBePositive, "Gas pressure in bar multiplied by cell length in metres");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(PropertyNames::COVARIANCE, "", Direction::Input,
                                                                       PropertyMode::Optional));
  declareProperty(PropertyNames::START_LAMBDA, 1.75, mustBePositive,
                  "Lower boundary of wavelength range to use for fitting");
  declareProperty(PropertyNames::END_LAMBDA, 8.0, mustBePositive,
                  "Upper boundary of wavelength range to use for fitting");
  declareProperty(PropertyNames::STOP_ON_FIT_ERROR, true, "If the fit fails, then stop the algorithm",
                  Direction::Input);
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

  ITableWorkspace_sptr covarianceMatrix = getProperty(PropertyNames::COVARIANCE);
  if (covarianceMatrix != nullptr) {
    // Should be a 2x2 matrix with a Name column
    const auto numRows = covarianceMatrix->rowCount();
    const auto numCols = covarianceMatrix->columnCount();
    // One extra column for Name
    if (numCols != 3 || numRows != 2) {
      errorList[PropertyNames::COVARIANCE] =
          "The covariance matrix is the wrong size, it should be a 2x2 matrix containing "
          "the T_E and pxd covariance matrix, with an extra column for Name.";
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
  std::string spinConfigurationInput = getProperty(PropertyNames::SPIN_STATES);

  const auto t11Ws =
      SpinStateValidator::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput, SpinStateValidator::ONE_ONE);
  const auto t10Ws =
      SpinStateValidator::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput, SpinStateValidator::ONE_ZERO);
  const auto t01Ws =
      SpinStateValidator::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput, SpinStateValidator::ZERO_ONE);
  const auto t00Ws =
      SpinStateValidator::WorkspaceForSpinState(groupWorkspace, spinConfigurationInput, SpinStateValidator::ZERO_ZERO);

  // T_NSF = T11 + T00 (NSF = not spin flipped)
  MatrixWorkspace_sptr tnsfWs = addTwoWorkspaces(t11Ws, t00Ws);

  // T_SF = T01 + T10 (SF = spin flipped)
  MatrixWorkspace_sptr tsfWs = addTwoWorkspaces(t01Ws, t10Ws);

  // P = tanh(mu * phe) where P is the polarisation of an unpolarised incoming beam
  // after the analyser cell. We're going to calculate P from the data,
  // P = (T_NSF - T_SF) / (T_NSF + T_SF), then fit tanh(mu * phe) to it in order
  // to calculate phe.
  MatrixWorkspace_sptr denom = addTwoWorkspaces(tnsfWs, tsfWs);
  MatrixWorkspace_sptr numerator = subtractWorkspaces(tnsfWs, tsfWs);
  MatrixWorkspace_sptr p = divideWorkspace(numerator, denom);

  // Now we fit tanh(mu*pHe*x) to P to give us pHe

  const double pxd = getProperty(PropertyNames::PXD);
  const double mu = ABSORPTION_CROSS_SECTION_CONSTANT * pxd;

  double pHe, pHeError;
  std::vector<double> wavelengthValues;
  fitAnalyserEfficiency(mu, p, pHe, pHeError, wavelengthValues);

  auto createSingleValuedWorkspace = createChildAlgorithm("CreateSingleValuedWorkspace");
  createSingleValuedWorkspace->initialize();
  createSingleValuedWorkspace->setProperty("DataValue", pHe);
  createSingleValuedWorkspace->setProperty("ErrorValue", pHeError);
  createSingleValuedWorkspace->setProperty("OutputWorkspace", "phe");
  createSingleValuedWorkspace->execute();
  MatrixWorkspace_sptr pheWs = createSingleValuedWorkspace->getProperty("OutputWorkspace");

  setProperty(PropertyNames::P_HE, pheWs);

  // Now we have all the parameters to calculate T(lambda), the transmission of the helium
  // analyser for an incident unpolarised beam. T_para and T_anti are also calculated, the
  // transmission of the wanted and unwanted spin state. T = (T_para + T_anti) / 2

  MantidVec tPara, tAnti, tParaErrors, tAntiErrors;
  calculateTransmission(wavelengthValues, pHe, pHeError, mu, tPara, tAnti, tParaErrors, tAntiErrors);

  MatrixWorkspace_sptr tParaWorkspace =
      createWorkspace("tPara", "Helium Analyser Transmission T_para", wavelengthValues, tPara, tParaErrors);
  setProperty(PropertyNames::OUTPUT_T_PARA_WORKSPACE, tParaWorkspace);

  MatrixWorkspace_sptr tAntiWorkspace =
      createWorkspace("tAnti", "Helium Analyser Transmission T_anti", wavelengthValues, tAnti, tAntiErrors);
  setProperty(PropertyNames::OUTPUT_T_ANTI_WORKSPACE, tAntiWorkspace);

  MatrixWorkspace_sptr transmissionWorkspace = addTwoWorkspaces(tParaWorkspace, tAntiWorkspace);

  auto scale = createChildAlgorithm("Scale");
  scale->initialize();
  scale->setProperty("InputWorkspace", transmissionWorkspace);
  scale->setProperty("OutputWorkspace", transmissionWorkspace);
  scale->setProperty("Factor", 0.5);
  scale->setProperty("Operation", "Multiply");
  scale->execute();

  setProperty(PropertyNames::OUTPUT_WORKSPACE, transmissionWorkspace);
}

void HeliumAnalyserEfficiency::fitAnalyserEfficiency(const double &mu, MatrixWorkspace_sptr p, double &pHe,
                                                     double &pHeError, MantidVec &wavelengthValues) {
  auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", "name=UserFunction,Formula=tanh(" + std::to_string(mu) + "*phe*x),phe=0.1");
  fit->setProperty("InputWorkspace", p);
  const double startLambda = getProperty("StartLambda");
  fit->setProperty("StartX", startLambda);
  const double endLambda = getProperty("EndLambda");
  fit->setProperty("EndX", endLambda);
  fit->setProperty("CreateOutput", true);
  fit->execute();

  const bool stopOnFitError = getProperty(PropertyNames::STOP_ON_FIT_ERROR);
  const std::string &status = fit->getProperty("OutputStatus");
  if (stopOnFitError && (!fit->isExecuted() || status != "success")) {
    auto const &errMsg{"Failed to fit to data in the calculation of p_He: " + status};
    g_log.error(errMsg);
    throw std::runtime_error(errMsg);
  }

  ITableWorkspace_sptr fitParameters = fit->getProperty("OutputParameters");
  MatrixWorkspace_sptr fitWorkspace = fit->getProperty("OutputWorkspace");

  pHe = fitParameters->getRef<double>("Value", 0);
  pHeError = fitParameters->getRef<double>("Error", 0);
  const auto wavelengthValuesHist = fitWorkspace->x(0);
  wavelengthValues = MantidVec(wavelengthValuesHist.cbegin(), wavelengthValuesHist.cend());
}

void HeliumAnalyserEfficiency::calculateTransmission(const MantidVec &wavelengthValues, const double &pHe,
                                                     const double &pHeError, const double &mu, MantidVec &tPara,
                                                     MantidVec &tAnti, MantidVec &tParaErrors, MantidVec &tAntiErrors) {
  tPara = MantidVec(wavelengthValues.size());
  tAnti = MantidVec(wavelengthValues.size());
  tParaErrors = MantidVec(wavelengthValues.size());
  tAntiErrors = MantidVec(wavelengthValues.size());

  double s00, s01, s10, s11;
  s00 = s01 = s10 = s11 = 0;
  ITableWorkspace_sptr covarianceMatrix = getProperty(PropertyNames::COVARIANCE);
  if (covarianceMatrix != nullptr) {
    s00 = covarianceMatrix->cell<double>(0, 1);
    s01 = covarianceMatrix->cell<double>(0, 2);
    s10 = covarianceMatrix->cell<double>(1, 1);
    s11 = covarianceMatrix->cell<double>(1, 2);
  }

  const double pHe_variance = pHeError * pHeError;
  const double t_E = getProperty(PropertyNames::T_E);

  // Create a t distribution with dof given by the number of data points minus
  // the number of params (3)
  double tPpf = 1;
  if (wavelengthValues.size() > 3) {
    const boost::math::students_t dist(static_cast<double>(wavelengthValues.size()) - 3.0);
    // Critical value corresponding to 1-sigma
    const double alpha = (1 + std::erf(1.0 / sqrt(2))) / 2;
    // Scale factor for the error calculations
    tPpf = boost::math::quantile(dist, alpha);
  } else {
    g_log.warning(
        "The number of histogram bins must be greater than 3 in order to provide an accurate error calculation");
  }

  // This is the error calculation for T_para and T_anti using the error on pHe and
  // the supplied covariance matrix (if there is one).

  for (size_t i = 0; i < wavelengthValues.size(); ++i) {
    const double w = wavelengthValues[i];
    tPara[i] = 0.5 * t_E * std::exp(-mu * w * (1 - pHe));
    const double dTp_dpHe = mu * w * tPara[i];
    const double dTp_dT_E = tPara[i] / t_E;
    const double dTp_dpxd = -ABSORPTION_CROSS_SECTION_CONSTANT * w * (1 - pHe) * tPara[i];
    tParaErrors[i] =
        tPpf * std::sqrt(dTp_dpHe * dTp_dpHe * pHe_variance + dTp_dT_E * dTp_dT_E * s00 + dTp_dT_E * dTp_dpxd * s01 +
                         dTp_dpxd * dTp_dT_E * s10 + dTp_dpxd * dTp_dpxd * s11);
    tAnti[i] = 0.5 * t_E * std::exp(-mu * w * (1 + pHe));
    const double dTa_dpHe = mu * w * tAnti[i];
    const double dTa_dT_E = tAnti[i] / t_E;
    const double dTa_dpxd = -ABSORPTION_CROSS_SECTION_CONSTANT * w * (1 + pHe) * tAnti[i];
    tAntiErrors[i] =
        tPpf * std::sqrt(dTa_dpHe * dTa_dpHe * pHe_variance + dTa_dT_E * dTa_dT_E * s00 + dTa_dT_E * dTa_dpxd * s01 +
                         dTa_dpxd * dTa_dT_E * s10 + dTa_dpxd * dTa_dpxd * s11);
  }
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
                                                               const MantidVec &eData, const std::string &xUnit) {
  auto createWorkspace = createChildAlgorithm("CreateWorkspace");
  createWorkspace->initialize();
  createWorkspace->setProperty("OutputWorkspace", name);
  createWorkspace->setProperty("DataX", xData);
  createWorkspace->setProperty("DataY", yData);
  createWorkspace->setProperty("DataE", eData);
  createWorkspace->setProperty("UnitX", xUnit);
  createWorkspace->setProperty("WorkspaceTitle", title);
  createWorkspace->execute();
  return createWorkspace->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr HeliumAnalyserEfficiency::subtractWorkspaces(MatrixWorkspace_sptr ws,
                                                                  MatrixWorkspace_sptr wsToSubtract) {
  auto minus = createChildAlgorithm("Minus");
  minus->initialize();
  minus->setProperty("LHSWorkspace", ws);
  minus->setProperty("RHSWorkspace", wsToSubtract);
  minus->setProperty("OutputWorkspace", "minus");
  minus->execute();
  return minus->getProperty("OutputWorkspace");
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

const double HeliumAnalyserEfficiency::ABSORPTION_CROSS_SECTION_CONSTANT = 0.0733;
} // namespace Mantid::Algorithms