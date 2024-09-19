// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiency.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"

#include <algorithm>
#include <boost/math/distributions/students_t.hpp>
#include <vector>

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(HeliumAnalyserEfficiency)

using namespace Kernel;
using namespace API;

constexpr double HeliumAnalyserEfficiency::ABSORPTION_CROSS_SECTION_CONSTANT = 0.0733;

namespace PropertyNames {
auto constexpr INPUT_WORKSPACE{"InputWorkspace"};
auto constexpr OUTPUT_WORKSPACE{"OutputWorkspace"};
auto constexpr OUTPUT_FIT_CURVES{"OutputFitCurves"};
auto constexpr OUTPUT_FIT_PARAMS{"OutputFitParameters"};
auto constexpr SPIN_STATES{"SpinStates"};
auto constexpr PD{"GasPressureTimesCellLength"};
auto constexpr PD_ERROR{"GasPressureTimesCellLengthError"};
auto constexpr START_LAMBDA{"StartLambda"};
auto constexpr END_LAMBDA{"EndLambda"};
auto constexpr IGNORE_FIT_QUALITY_ERROR{"IgnoreFitQualityError"};

auto constexpr GROUP_INPUTS{"Inputs"};
auto constexpr GROUP_FIT_OPTIONS{"Fit Options"};
auto constexpr GROUP_OUTPUTS{"Outputs"};
} // namespace PropertyNames

void HeliumAnalyserEfficiency::init() {
  // Declare required input parameters for algorithm and do some validation here
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input),
      "Input group workspace to use for polarization calculation");

  auto spinValidator = std::make_shared<SpinStateValidator>(std::unordered_set<int>{4});
  declareProperty(PropertyNames::SPIN_STATES, "11,10,01,00", spinValidator,
                  "Order of individual flipper configurations in the input group workspace, e.g. \"01,11,00,10\"");

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
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
                  "Helium analyzer efficiency as a function of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(std::string(PropertyNames::OUTPUT_FIT_CURVES),
                                                                       "", Kernel::Direction::Output,
                                                                       PropertyMode::Optional),
                  "The name of the matrix workspace containing the calculated fit curve, the original data, and the "
                  "difference between the two.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(std::string(PropertyNames::OUTPUT_FIT_PARAMS),
                                                                       "", Kernel::Direction::Output,
                                                                       PropertyMode::Optional),
                  "The name of the table workspace containing the fit parameter results.");

  setPropertyGroup(PropertyNames::SPIN_STATES, PropertyNames::GROUP_INPUTS);
  setPropertyGroup(PropertyNames::PD, PropertyNames::GROUP_INPUTS);
  setPropertyGroup(PropertyNames::PD_ERROR, PropertyNames::GROUP_INPUTS);

  setPropertyGroup(PropertyNames::START_LAMBDA, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::END_LAMBDA, PropertyNames::GROUP_FIT_OPTIONS);
  setPropertyGroup(PropertyNames::IGNORE_FIT_QUALITY_ERROR, PropertyNames::GROUP_FIT_OPTIONS);

  setPropertyGroup(PropertyNames::OUTPUT_WORKSPACE, PropertyNames::GROUP_OUTPUTS);
  setPropertyGroup(PropertyNames::OUTPUT_FIT_CURVES, PropertyNames::GROUP_OUTPUTS);
  setPropertyGroup(PropertyNames::OUTPUT_FIT_PARAMS, PropertyNames::GROUP_OUTPUTS);
}

void validateInputWorkspace(MatrixWorkspace_sptr const &workspace, std::map<std::string, std::string> &errorList) {
  if (workspace == nullptr) {
    errorList[PropertyNames::INPUT_WORKSPACE] = "All input workspaces must be of type MatrixWorkspace.";
    return;
  }

  Kernel::Unit_const_sptr unit = workspace->getAxis(0)->unit();
  if (unit->unitID() != "Wavelength") {
    errorList[PropertyNames::INPUT_WORKSPACE] = "All input workspaces must be in units of Wavelength.";
  }
}
/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an error message
 */
std::map<std::string, std::string> HeliumAnalyserEfficiency::validateInputs() {
  std::map<std::string, std::string> errorList;
  const WorkspaceGroup_sptr wsGroup = getProperty(PropertyNames::INPUT_WORKSPACE);

  if (wsGroup == nullptr) {
    errorList[PropertyNames::INPUT_WORKSPACE] = "The input workspace is not a group workspace";
  } else if (wsGroup->size() != 4) {
    errorList[PropertyNames::INPUT_WORKSPACE] =
        "The input group workspace must have four periods corresponding to the four spin configurations.";
  } else {
    for (size_t i = 0; i < wsGroup->size(); ++i) {
      const MatrixWorkspace_sptr stateWs = std::dynamic_pointer_cast<MatrixWorkspace>(wsGroup->getItem(i));
      validateInputWorkspace(stateWs, errorList);
    }
  }
  return errorList;
}

void HeliumAnalyserEfficiency::exec() {
  MatrixWorkspace_sptr eff = calculateAnalyserEfficiency();

  // Theoretically, the analyser efficiency is given by (1 + tanh(mu * phe * wavelength))/2.
  // Using the analyser efficiency value that we calculated from the data,
  // we fit this function to find pHe, the helium atom polarization in the analyser.
  const double mu = ABSORPTION_CROSS_SECTION_CONSTANT * static_cast<double>(getProperty(PropertyNames::PD));

  double pHe, pHeError;
  fitAnalyserEfficiency(mu, eff, pHe, pHeError);

  // Now re-calculate the efficiency values in the workspace using the theoretical relationship with the fit result for
  // pHe. We do this because the efficiency calculated from the data will include inherent noise and structure coming
  // from the statistical noise, whereas the one calculated from the theory will give the expected smooth result.
  convertToTheoreticalEfficiency(eff, pHe, pHeError, mu);
  setProperty(PropertyNames::OUTPUT_WORKSPACE, eff);
}

MatrixWorkspace_sptr HeliumAnalyserEfficiency::calculateAnalyserEfficiency() {
  // First we extract the individual workspaces corresponding to each spin configuration from the group workspace
  const WorkspaceGroup_sptr groupWorkspace = getProperty(PropertyNames::INPUT_WORKSPACE);
  const std::string spinConfigurationInput = getProperty(PropertyNames::SPIN_STATES);

  const auto t11Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           FlipperConfigurations::ON_ON);
  const auto t10Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           FlipperConfigurations::ON_OFF);
  const auto t01Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           FlipperConfigurations::OFF_ON);
  const auto t00Ws = PolarizationCorrectionsHelpers::workspaceForSpinState(groupWorkspace, spinConfigurationInput,
                                                                           FlipperConfigurations::OFF_OFF);

  // T_NSF = T11 + T00 (NSF = not spin flipped)
  MatrixWorkspace_sptr tnsfWs = t11Ws + t00Ws;

  // T_SF = T01 + T10 (SF = spin flipped)
  MatrixWorkspace_sptr tsfWs = t01Ws + t10Ws;

  // Calculate the analyser efficiency from the data, eff = T_NSF / (T_NSF + T_SF)
  return tnsfWs / (tnsfWs + tsfWs);
}

void HeliumAnalyserEfficiency::fitAnalyserEfficiency(const double mu, const MatrixWorkspace_sptr eff, double &pHe,
                                                     double &pHeError) {
  auto fit = createChildAlgorithm("Fit");
  fit->initialize();
  fit->setProperty("Function", "name=UserFunction,Formula=(1 + tanh(" + std::to_string(mu) + "*phe*x))/2,phe=0.1");
  fit->setProperty("InputWorkspace", eff);
  const double startLambda = getProperty(PropertyNames::START_LAMBDA);
  fit->setProperty("StartX", startLambda);
  const double endLambda = getProperty(PropertyNames::END_LAMBDA);
  fit->setProperty("EndX", endLambda);
  fit->setProperty("CreateOutput", true);
  fit->execute();

  const bool ignoreFitQualityError = getProperty(PropertyNames::IGNORE_FIT_QUALITY_ERROR);
  const std::string &status = fit->getProperty("OutputStatus");
  if (!ignoreFitQualityError && (!fit->isExecuted() || status != "success")) {
    throw std::runtime_error("Failed to fit to data in the calculation of p_He: " + status);
  }

  const ITableWorkspace_sptr fitParameters = fit->getProperty("OutputParameters");

  if (!getPropertyValue(PropertyNames::OUTPUT_FIT_PARAMS).empty()) {
    setProperty(PropertyNames::OUTPUT_FIT_PARAMS, fitParameters);
  }
  if (!API::Algorithm::getPropertyValue(PropertyNames::OUTPUT_FIT_CURVES).empty()) {
    const MatrixWorkspace_sptr fitWorkspace = fit->getProperty("OutputWorkspace");
    setProperty(PropertyNames::OUTPUT_FIT_CURVES, fitWorkspace);
  }

  pHe = fitParameters->getRef<double>("Value", 0);
  pHeError = fitParameters->getRef<double>("Error", 0);
}

void HeliumAnalyserEfficiency::convertToTheoreticalEfficiency(MatrixWorkspace_sptr &eff, const double pHe,
                                                              const double pHeError, const double mu) {
  const auto &pointData = eff->histogram(0).points();
  const auto &binPoints = pointData.rawData();
  const auto &binBoundaries = eff->x(0);

  auto &theoreticalEff = eff->mutableY(0);
  auto &efficiencyErrors = eff->mutableE(0);

  // The value tCrit is used to give us the correct error bounds
  const double tCrit = calculateTCrit(eff->blocksize());
  const double muError = ABSORPTION_CROSS_SECTION_CONSTANT * static_cast<double>(getProperty(PropertyNames::PD_ERROR));

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

double HeliumAnalyserEfficiency::calculateTCrit(const size_t numberOfBins) {
  // Create a t distribution with dof given by the number of data points minus
  // the number of params (2)
  double tPpf = 1;
  if (numberOfBins > 2) {
    const boost::math::students_t dist(static_cast<double>(numberOfBins) - 2.0);
    // Critical value corresponding to 1-sigma
    const double alpha = (1 + std::erf(1.0 / sqrt(2))) / 2;
    // Scale factor for the error calculations
    tPpf = boost::math::quantile(dist, alpha);
  } else {
    g_log.warning(
        "The number of histogram bins must be greater than 2 in order to provide an accurate error calculation");
  }
  return tPpf;
}
} // namespace Mantid::Algorithms