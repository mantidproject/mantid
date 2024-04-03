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
static const std::string P_CELL = "AnalyserPolarization";
static const std::string P_HE = "HeliumAtomsPolarization";
static const std::string OUTPUT_T_WORKSPACE = "OutputTransmissionWorkspace";
static const std::string OUTPUT_T_PARA_WORKSPACE = "OutputTransmissionParaWorkspace";
static const std::string OUTPUT_T_ANTI_WORKSPACE = "OutputTransmissionAntiWorkspace";
static const std::string SPIN_STATES = "SpinStates";
static const std::string T_E = "TransmissionEmptyCell";
static const std::string PXD = "GasPressureTimesCellLength";
static const std::string COVARIANCE = "Covariance";
static const std::string START_LAMBDA = "StartLambda";
static const std::string END_LAMBDA = "EndLambda";
static const std::string IGNORE_FIT_QUALITY_ERROR = "IgnoreFitQualityError";
} // namespace PropertyNames

void HeliumAnalyserEfficiency::init() {
  // Declare required input parameters for algorithm and do some validation here
  auto validator = std::make_shared<CompositeValidator>();
  validator->add<WorkspaceUnitValidator>("Wavelength");
  validator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input, validator),
      "Input group workspace to use for polarization calculation");
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
      "Helium analyzer efficiency as a function of wavelength");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::P_CELL, "", Direction::Output, PropertyMode::Optional),
      "Helium analyser polarization as a function of wavelength");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::P_HE, "", Direction::Output, PropertyMode::Optional),
      "Helium atoms polarization, a single value");
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_T_WORKSPACE, "", Direction::Output,
                                                        PropertyMode::Optional),
                  "Incident neutron transmission through the analyser as a function of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_T_PARA_WORKSPACE, "", Direction::Output,
                                                        PropertyMode::Optional),
                  "Parallel neutron transmission through the analyser as a function of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_T_ANTI_WORKSPACE, "", Direction::Output,
                                                        PropertyMode::Optional),
                  "Antiparallel neutron transmission through the analyser as a function of wavelength");

  auto spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  std::string initialSpinConfig = "11,10,01,00";
  declareProperty(PropertyNames::SPIN_STATES, initialSpinConfig, spinValidator,
                  "Order of individual spin states in the input group workspace, e.g. \"01,11,00,10\"");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(PropertyNames::T_E, 0.9, mustBePositive, "Transmission of the empty cell");
  declareProperty(PropertyNames::PXD, 12.0, mustBePositive, "Gas pressure in bar multiplied by cell length in metres");
  declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>(PropertyNames::COVARIANCE, "", Direction::Input,
                                                           PropertyMode::Optional),
      "Covariance matrix for the transmission of the empty cell and the gas pressure multiplied by cell length");
  declareProperty(PropertyNames::START_LAMBDA, 1.75, mustBePositive,
                  "Lower boundary of wavelength range to use for fitting");
  declareProperty(PropertyNames::END_LAMBDA, 8.0, mustBePositive,
                  "Upper boundary of wavelength range to use for fitting");
  declareProperty(PropertyNames::IGNORE_FIT_QUALITY_ERROR, false,
                  "Whether the algorithm should ignore a poor chi-squared (fit cost value) of greater than 1 and "
                  "therefore not throw an error",
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
  MantidVec wavelengthValues, pCalc;
  fitAnalyserEfficiency(mu, p, pHe, pHeError, wavelengthValues, pCalc);

  // This value is used to give us the correct error bounds
  const double tCrit = calculateTCrit(wavelengthValues.size());

  // Analyser cell polarization
  auto pCalcWorkspace = createPolarizationWorkspace(pxd, pHe, pHeError, tCrit, wavelengthValues, pCalc);
  if (!getPropertyValue(PropertyNames::P_CELL).empty()) {
    setProperty(PropertyNames::P_CELL, pCalcWorkspace);
  }

  // Now we can finally calculate the efficiencies
  auto efficiencies = calculateEfficiencies(pCalcWorkspace, groupWorkspace, spinConfigurationInput);
  setProperty(PropertyNames::OUTPUT_WORKSPACE, efficiencies);

  setOptionalOutputProperties(wavelengthValues, pHe, pHeError, mu, tCrit);
}

WorkspaceGroup_sptr HeliumAnalyserEfficiency::calculateEfficiencies(MatrixWorkspace_sptr pCell,
                                                                    WorkspaceGroup_sptr inputGroup,
                                                                    const std::string &spinStateOrder) {
  // Need the parallel and antiparallel efficiencies, then put them in the same order
  // as the specified spin state order of the input group workspace
  const auto numPoints = pCell->dataY(0).size();
  auto eParallel = std::vector<double>(numPoints);
  auto eAnti = std::vector<double>(numPoints);
  auto eError = std::vector<double>(numPoints);
  const auto pCellY = pCell->dataY(0);
  const auto pCellError = pCell->dataE(0);
  for (size_t i = 0; i < numPoints; ++i) {
    eParallel[i] = (1 + pCellY[i]) / 2.0;
    eAnti[i] = (1 - pCellY[i]) / 2.0;
    eError[i] = pCellError[i] / 2.0;
  }

  const std::string outputWorkspaceName = getPropertyValue(PropertyNames::OUTPUT_WORKSPACE);

  auto ws00 = createWorkspace(outputWorkspaceName + "00", "Analyser efficiency parallel", pCell->dataX(0), eParallel,
                              eError, true);
  auto ws01 = createWorkspace(outputWorkspaceName + "01", "Analyser efficiency antiparallel", pCell->dataX(0), eAnti,
                              eError, true);
  auto ws10 = createWorkspace(outputWorkspaceName + "10", "Analyser efficiency antiparallel", pCell->dataX(0), eAnti,
                              eError, true);
  auto ws11 = createWorkspace(outputWorkspaceName + "11", "Analyser efficiency parallel", pCell->dataX(0), eParallel,
                              eError, true);

  const auto ws00Index = PolarizationCorrectionsHelpers::IndexOfWorkspaceForSpinState(inputGroup, spinStateOrder,
                                                                                      SpinStateValidator::ZERO_ZERO);
  const auto ws01Index = PolarizationCorrectionsHelpers::IndexOfWorkspaceForSpinState(inputGroup, spinStateOrder,
                                                                                      SpinStateValidator::ZERO_ONE);
  const auto ws10Index = PolarizationCorrectionsHelpers::IndexOfWorkspaceForSpinState(inputGroup, spinStateOrder,
                                                                                      SpinStateValidator::ONE_ZERO);
  const auto ws11Index = PolarizationCorrectionsHelpers::IndexOfWorkspaceForSpinState(inputGroup, spinStateOrder,
                                                                                      SpinStateValidator::ONE_ONE);

  auto wsVector = std::vector<MatrixWorkspace_sptr>(4);
  wsVector[ws00Index] = ws00;
  wsVector[ws01Index] = ws01;
  wsVector[ws10Index] = ws10;
  wsVector[ws11Index] = ws11;

  auto groupWorkspace = createChildAlgorithm("GroupWorkspaces");
  groupWorkspace->initialize();
  std::vector<std::string> wsToGroupNames(4);
  std::transform(wsVector.cbegin(), wsVector.cend(), wsToGroupNames.begin(),
                 [](MatrixWorkspace_sptr w) { return w->getName(); });
  groupWorkspace->setProperty("InputWorkspaces", wsToGroupNames);
  groupWorkspace->setProperty("OutputWorkspace", outputWorkspaceName);
  groupWorkspace->execute();

  WorkspaceGroup_sptr wsGrp = groupWorkspace->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(outputWorkspaceName, wsGrp);
  return wsGrp;
}

void HeliumAnalyserEfficiency::setOptionalOutputProperties(const MantidVec &wavelengthValues, const double pHe,
                                                           const double pHeError, const double mu, const double tCrit) {
  if (!getPropertyValue(PropertyNames::P_HE).empty()) {
    auto createSingleValuedWorkspace = createChildAlgorithm("CreateSingleValuedWorkspace");
    createSingleValuedWorkspace->initialize();
    createSingleValuedWorkspace->setProperty("DataValue", pHe);
    createSingleValuedWorkspace->setProperty("ErrorValue", pHeError);
    createSingleValuedWorkspace->setProperty("OutputWorkspace", "phe");
    createSingleValuedWorkspace->execute();
    MatrixWorkspace_sptr pheWs = createSingleValuedWorkspace->getProperty("OutputWorkspace");
    setProperty(PropertyNames::P_HE, pheWs);
  }

  // Now we have all the parameters to calculate T(lambda), the transmission of the helium
  // analyser for an incident unpolarised beam. T_para and T_anti are also calculated, the
  // transmission of the wanted and unwanted spin state. T = (T_para + T_anti) / 2

  if (getPropertyValue(PropertyNames::OUTPUT_T_WORKSPACE).empty() &&
      getPropertyValue(PropertyNames::OUTPUT_T_PARA_WORKSPACE).empty() &&
      getPropertyValue(PropertyNames::OUTPUT_T_ANTI_WORKSPACE).empty()) {
    return;
  }

  MantidVec tPara, tAnti, tParaErrors, tAntiErrors;
  calculateTransmission(wavelengthValues, pHe, pHeError, mu, tCrit, tPara, tAnti, tParaErrors, tAntiErrors);

  MatrixWorkspace_sptr tParaWorkspace =
      createWorkspace("tPara", "Helium Analyser Transmission T_para", wavelengthValues, tPara, tParaErrors);
  if (!getPropertyValue(PropertyNames::OUTPUT_T_PARA_WORKSPACE).empty()) {
    setProperty(PropertyNames::OUTPUT_T_PARA_WORKSPACE, tParaWorkspace);
  }

  MatrixWorkspace_sptr tAntiWorkspace =
      createWorkspace("tAnti", "Helium Analyser Transmission T_anti", wavelengthValues, tAnti, tAntiErrors);
  if (!getPropertyValue(PropertyNames::OUTPUT_T_ANTI_WORKSPACE).empty()) {
    setProperty(PropertyNames::OUTPUT_T_ANTI_WORKSPACE, tAntiWorkspace);
  }

  if (getPropertyValue(PropertyNames::OUTPUT_T_WORKSPACE).empty()) {
    return;
  }

  MatrixWorkspace_sptr transmissionWorkspace = addTwoWorkspaces(tParaWorkspace, tAntiWorkspace);

  auto scale = createChildAlgorithm("Scale");
  scale->initialize();
  scale->setProperty("InputWorkspace", transmissionWorkspace);
  scale->setProperty("OutputWorkspace", transmissionWorkspace);
  scale->setProperty("Factor", 0.5);
  scale->setProperty("Operation", "Multiply");
  scale->execute();

  setProperty(PropertyNames::OUTPUT_T_WORKSPACE, transmissionWorkspace);
}

void HeliumAnalyserEfficiency::fitAnalyserEfficiency(const double mu, MatrixWorkspace_sptr p, double &pHe,
                                                     double &pHeError, MantidVec &wavelengthValues, MantidVec &pCalc) {
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

  const bool stopOnFitError = getProperty(PropertyNames::IGNORE_FIT_QUALITY_ERROR);
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
  pCalc = MantidVec(fitWorkspace->y(0).cbegin(), fitWorkspace->y(0).cend());
}

MatrixWorkspace_sptr HeliumAnalyserEfficiency::createPolarizationWorkspace(const double pd, const double pHe,
                                                                           const double pHeError, const double tCrit,
                                                                           const MantidVec &wavelengthValues,
                                                                           const MantidVec &pCalc) {
  // Calculate errors on the p curve from the pHe fit. We can't use the fit errors
  // directly because we also have an error on the pd term (which the fit doesn't
  // know about).

  auto pCalcError = MantidVec(pCalc.size());
  ITableWorkspace_sptr covarianceMatrix = getProperty(PropertyNames::COVARIANCE);
  double pdVariance = 0;
  if (covarianceMatrix != nullptr) {
    pdVariance = covarianceMatrix->cell<double>(1, 2);
  } else {
    g_log.warning("No error data found for " + PropertyNames::PXD +
                  ", which should be in the covariance workspace called " + PropertyNames::COVARIANCE);
  }
  for (size_t i = 0; i < pCalcError.size(); ++i) {
    const double absorptionFactor = HeliumAnalyserEfficiency::ABSORPTION_CROSS_SECTION_CONSTANT * wavelengthValues[i];
    pCalcError[i] = tCrit * absorptionFactor * std::sqrt(pHeError * pHeError * pd * pd + pdVariance * pHe * pHe) /
                    std::pow(std::cosh(absorptionFactor * pd * pHe), 2.0);
  }

  auto pCellInput = getPropertyValue(PropertyNames::P_CELL);
  const std::string pName = pCellInput.empty() ? "P_Cell" : pCellInput;

  return createWorkspace(pName, "Helium Analyser Efficiency", wavelengthValues, pCalc, pCalcError);
}

void HeliumAnalyserEfficiency::calculateTransmission(const MantidVec &wavelengthValues, const double pHe,
                                                     const double pHeError, const double mu, const double tCrit,
                                                     MantidVec &tPara, MantidVec &tAnti, MantidVec &tParaErrors,
                                                     MantidVec &tAntiErrors) {
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

  // This is the error calculation for T_para and T_anti using the error on pHe and
  // the supplied covariance matrix (if there is one).

  for (size_t i = 0; i < wavelengthValues.size(); ++i) {
    const double w = wavelengthValues[i];
    tPara[i] = 0.5 * t_E * std::exp(-mu * w * (1 - pHe));
    const double dTp_dpHe = mu * w * tPara[i];
    const double dTp_dT_E = tPara[i] / t_E;
    const double dTp_dpxd = -ABSORPTION_CROSS_SECTION_CONSTANT * w * (1 - pHe) * tPara[i];
    tParaErrors[i] =
        tCrit * std::sqrt(dTp_dpHe * dTp_dpHe * pHe_variance + dTp_dT_E * dTp_dT_E * s00 + dTp_dT_E * dTp_dpxd * s01 +
                          dTp_dpxd * dTp_dT_E * s10 + dTp_dpxd * dTp_dpxd * s11);
    tAnti[i] = 0.5 * t_E * std::exp(-mu * w * (1 + pHe));
    const double dTa_dpHe = mu * w * tAnti[i];
    const double dTa_dT_E = tAnti[i] / t_E;
    const double dTa_dpxd = -ABSORPTION_CROSS_SECTION_CONSTANT * w * (1 + pHe) * tAnti[i];
    tAntiErrors[i] =
        tCrit * std::sqrt(dTa_dpHe * dTa_dpHe * pHe_variance + dTa_dT_E * dTa_dT_E * s00 + dTa_dT_E * dTa_dpxd * s01 +
                          dTa_dpxd * dTa_dT_E * s10 + dTa_dpxd * dTa_dpxd * s11);
  }
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
} // namespace Mantid::Algorithms