// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiency.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"

#include "MantidAPI/MultiDomainFunction.h"


#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include <MantidDataObjects/Workspace2D.h>

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/SpinStateValidator.h"
#include "MantidKernel/Unit.h"

#include <algorithm>
#include <vector>

#include <boost/math/distributions/students_t.hpp>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(HeliumAnalyserEfficiency)

using namespace Kernel;
using namespace API;

constexpr double LAMBDA_CONVERSION_FACTOR = 0.0733;

namespace PropertyNames {
constexpr auto INPUT_WORKSPACES{"InputWorkspaces"};
constexpr auto OUTPUT_WORKSPACE{"OutputWorkspace"};
constexpr auto SPIN_STATES{"SpinStates"};

constexpr auto PXD{"PXD"};
constexpr auto PXD_ERROR{"PXDError"};
constexpr auto DECAY_TIME_INITIAL{"DecayTimeInitial"};
constexpr auto H3_POLARIZATION_INITIAL{"H3PolarizationInitial"};

constexpr auto START_WAVELENGTH{"StartWavelength"};
constexpr auto END_WAVELENGTH{"EndWavelength"};
constexpr auto IGNORE_FIT_QUALITY_ERROR{"IgnoreFitQualityError"};
constexpr auto OUTPUT_FIT_CURVES{"OutputFitCurves"};
constexpr auto OUTPUT_FIT_PARAMS{"OutputFitParameters"};

constexpr auto GROUP_INPUTS{"Inputs"};
constexpr auto GROUP_FIT_OPTIONS{"Fit Options"};
constexpr auto GROUP_OUTPUTS{"Outputs"};
} // namespace PropertyNames

namespace Fitting {
constexpr auto OUTPUT_HE3_FIT{"_He3_polarization"};
constexpr auto OUTPUT_DECAY_FIT{"_decay"};
constexpr auto INPUT_WORKSPACE{"InputWorkspace"};
constexpr auto START_X{"StartX"};
constexpr auto END_X{"EndX"};


/// Initial fitting function values.
constexpr double PXD_INITIAL = 12.0;
constexpr double PXD_ERROR_INITIAL = 0.0;
constexpr double START_WAVELENGTH_INITIAL = 1.75;
constexpr double END_WAVELENGTH_INITIAL = 8;
constexpr double H3_POLARIZATION_INITIAL = 0.6;
constexpr double DECAY_TIME_INITIAL = 54; // Hours

std::shared_ptr<IFunction> prepareExpDecayFunction(const double initialDecay, const double initialPolarization) {
  std::string funcStr = "name=ExpDecay,";
  const std::string params =
      "Lifetime=" + std::to_string(initialDecay) + "," + "Height=" + std::to_string(initialPolarization);
  funcStr += params;
  return FunctionFactory::Instance().createInitialized(funcStr);
}

std::shared_ptr<MultiDomainFunction> prepareEfficiencyFunc(const double mu, const size_t numberOfDomains) {
  const auto efficiencyFunc = "name=UserFunction,Formula=(1 + tanh(" + std::to_string(mu) + "*phe*x))/2";
  return FunctionFactory::Instance().createInitializedMultiDomainFunction(efficiencyFunc, numberOfDomains);
}

MatrixWorkspace_sptr createFitDecayWorkspace(const std::vector<double> &time, const std::vector<double> &timeErrors,
                                             const std::vector<double> &HePolarization,
                                             const std::vector<double> &HePolarizationErrors) {
  const HistogramData::Points xVals(time);
  const HistogramData::Frequencies yVals(HePolarization);
  const HistogramData::FrequencyStandardDeviations eVals(
      HePolarizationErrors.empty() ? std::vector<double>(HePolarization.size()) : HePolarizationErrors);
  auto retVal = std::make_shared<DataObjects::Workspace2D>();
  retVal->initialize(1, HistogramData::Histogram(xVals, yVals, eVals));
  retVal->setPointStandardDeviations(0, timeErrors.empty() ? std::vector<double>(time.size()) : timeErrors);
  return retVal;
}
} // namespace Fitting

namespace {
Mantid::Kernel::Logger logger("HeliumAnalyserEfficiency");
constexpr size_t NUM_FIT_PARAMS = 1;

MatrixWorkspace_sptr calculateAnalyserEfficiency(const WorkspaceGroup_sptr &groupWorkspace,
                                                 const std::string &spinStates) {
  using namespace PolarizationCorrectionsHelpers;
  using namespace FlipperConfigurations;

  const auto t11Ws = workspaceForSpinState(groupWorkspace, spinStates, ON_ON);
  const auto t10Ws = workspaceForSpinState(groupWorkspace, spinStates, ON_OFF);
  const auto t01Ws = workspaceForSpinState(groupWorkspace, spinStates, OFF_ON);
  const auto t00Ws = workspaceForSpinState(groupWorkspace, spinStates, OFF_OFF);

  // T_NSF = T11 + T00 (NSF = not spin flipped)
  const MatrixWorkspace_sptr tnsfWs = t11Ws + t00Ws;
  // T_SF = T01 + T10 (SF = spin flipped)
  const MatrixWorkspace_sptr tsfWs = t01Ws + t10Ws;

  // Calculate the analyser efficiency from the data, eff = T_NSF / (T_NSF + T_SF)
  return tnsfWs / (tnsfWs + tsfWs);
}

// We template this as we expect either a vector of MatrixWorkspace_sptr or TableWorkspace_sptr
template <typename T = MatrixWorkspace_sptr>
WorkspaceGroup_sptr prepareOutputGroup(const std::vector<T> &workspaces, const std::string &baseName,
                                       const std::string &suffix = "") {
  // If we are extracting the names for the curves, the last workspace of the input vector will
  // always correspond to the decay fit.
  // The first n-1 workspaces refers each to one of the He3 Polarization fit curves.
  auto outputName = [&](const size_t index) -> std::string {
    return suffix.empty() ? baseName + "_" + std::to_string(index)
                          : (workspaces.size() > 1 && index == workspaces.size() - 1
                                 ? baseName + Fitting::OUTPUT_DECAY_FIT + suffix + "_0"
                                 : baseName + Fitting::OUTPUT_HE3_FIT + suffix + "_" + std::to_string(index));
  };

  const auto group = std::make_shared<WorkspaceGroup>();
  for (size_t index = 0; index < workspaces.size(); index++) {
    AnalysisDataService::Instance().addOrReplace(outputName(index), workspaces.at(index));
    group->addWorkspace(workspaces.at(index));
  }
  return group;
}

} // unnamed namespace

HeliumAnalyserEfficiency::HeliumAnalyserEfficiency()
    : m_outputCurves{std::vector<MatrixWorkspace_sptr>()}, m_outputParameters{std::vector<ITableWorkspace_sptr>()} {};

void HeliumAnalyserEfficiency::declareInputProperties() {
  declareProperty(std::make_unique<ArrayProperty<std::string>>("InputWorkspaces", std::make_shared<ADSValidator>()),
                  "List of Polarized Transmission Group Workspaces. Each item on the list must be a workspace group "
                  "with 4 members, "
                  "each one representing a spin state.");
  const auto spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(
      PropertyNames::SPIN_STATES, "", spinValidator,
      "Order of individual spin configurations in the input group workspaces, e.g. \"01,11,00,10\", it is assumed"
      " that all input workspaces have the same spin order.");
}

void HeliumAnalyserEfficiency::declareFitProperties() {
  const auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);

  declareProperty(PropertyNames::PXD, Fitting::PXD_INITIAL, mustBePositive,
                  "Gas pressure in bar multiplied by cell length in metres");
  declareProperty(PropertyNames::PXD_ERROR, Fitting::PXD_ERROR_INITIAL, mustBePositive,
                  "Error in gas pressure (p x d)");
  declareProperty(PropertyNames::DECAY_TIME_INITIAL, Fitting::DECAY_TIME_INITIAL, mustBePositive,
                  "Initial decay time for He3 Polarization decay fit");
  declareProperty(PropertyNames::H3_POLARIZATION_INITIAL, Fitting::H3_POLARIZATION_INITIAL, mustBePositive,
                  "Initial polarization for He3 Polarization decay fit");
  declareProperty(PropertyNames::START_WAVELENGTH, Fitting::START_WAVELENGTH_INITIAL, mustBePositive,
                  "Lower boundary of wavelength range to use for fitting helium polarization");
  declareProperty(PropertyNames::END_WAVELENGTH, Fitting::END_WAVELENGTH_INITIAL, mustBePositive,
                  "Upper boundary of wavelength range to use for fitting helium polarization");
  declareProperty(PropertyNames::IGNORE_FIT_QUALITY_ERROR, false,
                  "Whether the algorithm should ignore a poor chi-squared (fit cost value) of greater than 1 and "
                  "therefore not throw an error");
}
void HeliumAnalyserEfficiency::declareOutputProperties() {

  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
      "Helium analyzer efficiency as a function of wavelength");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      PropertyNames::OUTPUT_FIT_CURVES, "", Kernel::Direction::Output, PropertyMode::Optional),
                  "A group workspace containing the fit curves.");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      PropertyNames::OUTPUT_FIT_PARAMS, "", Kernel::Direction::Output, PropertyMode::Optional),
                  "A group workspace containing the fit parameters.");
}

void HeliumAnalyserEfficiency::init() {
  declareInputProperties();
  declareFitProperties();
  declareOutputProperties();

  setPropertyGroup(PropertyNames::INPUT_WORKSPACES, PropertyNames::GROUP_INPUTS);
  setPropertyGroup(PropertyNames::SPIN_STATES, PropertyNames::GROUP_INPUTS);

  setPropertyGroup(PropertyNames::PXD, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::PXD_ERROR, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::DECAY_TIME_INITIAL, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::H3_POLARIZATION_INITIAL, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::START_WAVELENGTH, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::END_WAVELENGTH, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::IGNORE_FIT_QUALITY_ERROR, PropertyNames::GROUP_FIT_OPTIONS);

  setPropertyGroup(PropertyNames::OUTPUT_FIT_PARAMS, PropertyNames::GROUP_OUTPUTS);
  setPropertyGroup(PropertyNames::OUTPUT_FIT_CURVES, PropertyNames::GROUP_OUTPUTS);
  setPropertyGroup(PropertyNames::OUTPUT_WORKSPACE, PropertyNames::GROUP_OUTPUTS);
}

std::string validateInputWorkspace(const MatrixWorkspace_sptr &workspace) {
  const auto preText = "Workspace " + workspace->getName();
  std::string errorMessage;

  if (!workspace) {
    errorMessage = preText + " must be of type MatrixWorkspace. ";
    return errorMessage;
  }
  Kernel::Unit_const_sptr unit = workspace->getAxis(0)->unit();
  if (unit->unitID() != "Wavelength") {
    errorMessage = preText + " must be in units of Wavelength. ";
  }

  if (workspace->getNumberHistograms() != 1) {
    errorMessage += preText + " must contain a single histogram. ";
  }

  if (!workspace->isHistogramData()) {
    errorMessage += preText + " must be histogram data. ";
  }

  return errorMessage;
}
/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an
 * error message
 */
std::map<std::string, std::string> HeliumAnalyserEfficiency::validateInputs() {
  std::map<std::string, std::string> errorList;
  const std::vector<std::string> &inputWorkspaces = getProperty(PropertyNames::INPUT_WORKSPACES);
  for (const auto &wsName : inputWorkspaces) {
    const auto ws = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    if (!ws) {
      errorList[PropertyNames::INPUT_WORKSPACES] = "Workspace " + wsName + " is not a group workspace.";
      return errorList;
    }
    if (ws->size() != 4) {
      errorList[PropertyNames::INPUT_WORKSPACES] =
          "The input group workspace must have four periods corresponding to all four spin configurations.";
      return errorList;
    }

    for (size_t i = 0; i < ws->size(); i++) {
      const MatrixWorkspace_sptr stateWs = std::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(i));
      const auto errorMsg = validateInputWorkspace(stateWs);
      if (!errorMsg.empty()) {
        errorList[PropertyNames::INPUT_WORKSPACES] = errorList[PropertyNames::INPUT_WORKSPACES].empty()
                                                         ? errorMsg
                                                         : errorList[PropertyNames::INPUT_WORKSPACES] + errorMsg;
      }
    }
  }
  return errorList;
}

VectorPair HeliumAnalyserEfficiency::getTimeDifferences(const std::vector<std::string> &wsNames) {
  const auto timeDiff = createChildAlgorithm("TimeDifference");
  timeDiff->initialize();
  timeDiff->setProperty("InputWorkspaces", wsNames);

  timeDiff->execute();

  const ITableWorkspace_sptr table = timeDiff->getProperty("OutputWorkspace");
  const auto colThours = table->getColumn("hours");
  const auto colThoursErr = table->getColumn("hours_error");
  auto tHours = colThours->numeric_fill();
  auto tHoursErr = colThoursErr->numeric_fill();
  return std::make_pair(tHours, tHoursErr);
}

void HeliumAnalyserEfficiency::exec() {
  const std::vector<std::string> workspaceNames = getProperty(PropertyNames::INPUT_WORKSPACES);
  const std::string spinConfigurationInput = getProperty(PropertyNames::SPIN_STATES);
  const double mu = LAMBDA_CONVERSION_FACTOR * static_cast<double>(getProperty(PropertyNames::PXD));

  const auto efficiencies = calculateEfficiencies(workspaceNames, spinConfigurationInput);
  /*double pHe, pHeError;
  fitAnalyserEfficiency(mu, eff, pHe, pHeError);

  // Now re-calculate the efficiency values in the workspace using the theoretical relationship with the fit result for
  // pHe. We do this because the efficiency calculated from the data will include inherent noise and structure coming
  // from the statistical noise, whereas the one calculated from the theory will give the expected smooth result.
  convertToTheoreticalEfficiency(eff, pHe, pHeError, mu);
  */
  const auto [pHe, pHeError] = fitHe3Polarization(mu, efficiencies);
  convertToTheoreticalEfficiencies(efficiencies, pHe, pHeError, mu);
  if (efficiencies.size() > 1) {
    const auto [t, tError] = getTimeDifferences(workspaceNames);
    const auto fitDecayWorkspace = Fitting::createFitDecayWorkspace(t, tError, pHe, pHeError);
    fitDecayTime(fitDecayWorkspace);
  } else {
    logger.notice("Only one input workspace provided, polarization decay can't be fit as it is a 2 parameter fit.");
  }

  prepareOutputs(efficiencies);
}

void HeliumAnalyserEfficiency::prepareOutputs(const std::vector<MatrixWorkspace_sptr> &efficiencies) {
  if (const auto outputCurves = getPropertyValue(PropertyNames::OUTPUT_FIT_CURVES); !outputCurves.empty()) {
    setProperty(PropertyNames::OUTPUT_FIT_CURVES, prepareOutputGroup(m_outputCurves, outputCurves, "_curves"));
  }
  if (const auto outputParams = getPropertyValue(PropertyNames::OUTPUT_FIT_PARAMS); !outputParams.empty()) {
    setProperty(PropertyNames::OUTPUT_FIT_PARAMS, prepareOutputGroup(m_outputParameters, outputParams, "_parameters"));
  }
  setProperty(PropertyNames::OUTPUT_WORKSPACE,
              prepareOutputGroup(efficiencies, getPropertyValue(PropertyNames::OUTPUT_WORKSPACE)));
}

VectorPair HeliumAnalyserEfficiency::fitHe3Polarization(const double mu,
                                                        const std::vector<MatrixWorkspace_sptr> &efficiencies) {
  const auto numberOfDomains = efficiencies.size();
  const auto multiDomainFunc = Fitting::prepareEfficiencyFunc(mu, numberOfDomains);
  const auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", multiDomainFunc->asString());
  fit->setProperty(Fitting::INPUT_WORKSPACE, efficiencies.at(0));
  for (size_t index = 1; index < numberOfDomains; index++) {
    fit->setProperty("InputWorkspace_" + std::to_string(index), efficiencies.at(index));
  }

  fit->setProperty(Fitting::START_X, static_cast<double>(getProperty(PropertyNames::START_WAVELENGTH)));
  fit->setProperty(Fitting::END_X, static_cast<double>(getProperty(PropertyNames::END_WAVELENGTH)));
  makeFit(fit, Fitting::OUTPUT_HE3_FIT);

  const auto &fitParameters = std::dynamic_pointer_cast<ITableWorkspace>(m_outputParameters.at(0));
  const auto colPhe = fitParameters->getColumn("Value");
  const auto colPheError = fitParameters->getColumn("Error");
  const auto pHe = colPhe->numeric_fill(numberOfDomains);
  const auto pHeError = colPheError->numeric_fill(numberOfDomains);
  return std::make_pair(pHe, pHeError);
}

void HeliumAnalyserEfficiency::fitDecayTime(const MatrixWorkspace_sptr &workspace) {
  const auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  const double initialTau = getProperty(PropertyNames::DECAY_TIME_INITIAL);
  const double initialPol = getProperty(PropertyNames::H3_POLARIZATION_INITIAL);
  fit->setProperty("Function", Fitting::prepareExpDecayFunction(initialTau, initialPol));
  fit->setProperty(Fitting::INPUT_WORKSPACE, workspace);

  makeFit(fit, Fitting::OUTPUT_DECAY_FIT);
}

void HeliumAnalyserEfficiency::makeFit(const Algorithm_sptr &fitAlgorithm, const std::string &fitOutputName) {
  const auto extractParameters =
      !getPropertyValue(PropertyNames::OUTPUT_FIT_PARAMS).empty() || fitOutputName == Fitting::OUTPUT_HE3_FIT;
  const auto extractCurves = !getPropertyValue(PropertyNames::OUTPUT_FIT_CURVES).empty();
  const bool ignoreFitQualityError = getProperty(PropertyNames::IGNORE_FIT_QUALITY_ERROR);

  fitAlgorithm->setProperty("CreateOutput", extractParameters || extractCurves);
  fitAlgorithm->setProperty("OutputParametersOnly", !extractCurves);

  fitAlgorithm->execute();

  if (const std::string status = fitAlgorithm->getProperty("OutputStatus");
      !ignoreFitQualityError && (!fitAlgorithm->isExecuted() || status != "success")) {
    throw std::runtime_error("Failed to fit to data in the fitting of" + fitOutputName + " : " + status);
  }

  if (extractParameters) {
    const ITableWorkspace_sptr fitParameters = fitAlgorithm->getProperty("OutputParameters");
    m_outputParameters.push_back(fitParameters);
  }

  if (extractCurves) {
    // If output is a group, the name of the group will end with `Workspaces`.
    if (fitAlgorithm->getPropertyValue(PropertyNames::OUTPUT_WORKSPACE).ends_with('s')) {
      const WorkspaceGroup_sptr &fitCurves = fitAlgorithm->getProperty(PropertyNames::OUTPUT_WORKSPACE);
      for (int i = 0; i < fitCurves->getNumberOfEntries(); i++) {
        m_outputCurves.push_back(std::dynamic_pointer_cast<MatrixWorkspace>(fitCurves->getItem(i)));
      }
    } else {
      const MatrixWorkspace_sptr &fitCurve = fitAlgorithm->getProperty(PropertyNames::OUTPUT_WORKSPACE);
      m_outputCurves.push_back(fitCurve);
    }
  }
}

double HeliumAnalyserEfficiency::calculateTCrit(const size_t numberOfBins) const {
  // Create a t distribution with degrees of freedom given by the number of data points minus
  // the number of parameters that were fitted for
  double tPpf = 1;
  if (numberOfBins > NUM_FIT_PARAMS) {
    const boost::math::students_t dist(static_cast<double>(numberOfBins) - static_cast<double>(NUM_FIT_PARAMS));
    // Critical value corresponding to 1-sigma
    const double alpha = (1 + std::erf(1.0 / sqrt(2))) / 2;
    // Scale factor for the error calculations
    tPpf = boost::math::quantile(dist, alpha);
  } else {
    logger.warning("The number of histogram bins must be greater than " + std::to_string(NUM_FIT_PARAMS) +
                   " in order to provide an accurate error calculation");
  }
  return tPpf;
}

std::vector<MatrixWorkspace_sptr>
HeliumAnalyserEfficiency::calculateEfficiencies(const std::vector<std::string> &workspaceNames,
                                                const std::string &spinConfiguration) {
  std::vector<MatrixWorkspace_sptr> efficiencies(workspaceNames.size());
  for (size_t index = 0; index < workspaceNames.size(); index++) {
    const auto inputGroupWs = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(workspaceNames.at(index));
    efficiencies.at(index) = calculateAnalyserEfficiency(inputGroupWs, spinConfiguration);
  }

  return efficiencies;
}

void HeliumAnalyserEfficiency::convertToTheoreticalEfficiencies(const std::vector<MatrixWorkspace_sptr> &efficiencies,
                                                                const std::vector<double> &pHeVec,
                                                                const std::vector<double> &pHeErrorVec,
                                                                const double mu) {
  const double muError = LAMBDA_CONVERSION_FACTOR * static_cast<double>(getProperty(PropertyNames::PXD_ERROR));

  for (size_t index = 0; index < efficiencies.size(); index++) {
    const auto pHe = pHeVec.at(index);
    const auto pHeError = pHeErrorVec.at(index);
    const auto eff = efficiencies.at(index);

    const auto &pointData = eff->histogram(0).points();
    const auto &binPoints = pointData.rawData();
    const auto &binBoundaries = eff->x(0);

    auto &theoreticalEff = eff->mutableY(0);
    auto &efficiencyErrors = eff->mutableE(0);

    // The value tCrit is used to give us the correct error bounds
    const double tCrit = calculateTCrit(eff->blocksize());
    for (size_t i = 0; i < binPoints.size(); ++i) {
      const double lambda = binPoints[i];
      const double lambdaError = binBoundaries[i + 1] - binBoundaries[i];

      theoreticalEff[i] = (1 + std::tanh(mu * pHe * lambda)) / 2.0;

      // Calculate the errors for the efficiency
      const auto commonTerm = 0.5 / std::pow(std::cosh(mu * lambda * pHe), 2);
      const double de_dpHe = mu * commonTerm * lambda;
      const double de_dmu = pHe * commonTerm * lambda;
      const double de_dlambda = mu * pHe * commonTerm;
      // Covariance between p_He and mu is zero
      efficiencyErrors[i] =
          tCrit * std::sqrt(de_dpHe * de_dpHe * pHeError * pHeError + de_dmu * de_dmu * muError * muError +
                            de_dlambda * de_dlambda * lambdaError * lambdaError);
    }
  }
}
} // namespace Mantid::Algorithms
