// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateIqt.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/numeric/conversion/cast.hpp>
#include <cmath>
#include <functional>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

namespace {
constexpr int DEFAULT_ITERATIONS = 100;
constexpr int DEFAULT_SEED = 89631139;

std::string createRebinString(double minimum, double maximum, double width) {
  std::stringstream rebinStream;
  rebinStream.precision(14);
  rebinStream << minimum << ", " << width << ", " << maximum;
  return rebinStream.str();
}

template <typename Generator>
void randomizeHistogramWithinError(HistogramY &row, const HistogramE &errors,
                                   Generator &generator) {
  for (auto i = 0u; i < row.size(); ++i)
    row[i] += generator(errors[i]);
}

MatrixWorkspace_sptr
randomizeWorkspaceWithinError(MatrixWorkspace_sptr workspace,
                              MersenneTwister &mTwister) {
  auto randomNumberGenerator = [&mTwister](const double error) {
    return mTwister.nextValue(-error, error);
  };
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
    randomizeHistogramWithinError(workspace->mutableY(i), workspace->e(i),
                                  randomNumberGenerator);
  return workspace;
}

double standardDeviation(const std::vector<double> &inputValues) {
  const auto inputSize = boost::numeric_cast<double>(inputValues.size());
  const auto mean =
      std::accumulate(inputValues.begin(), inputValues.end(), 0.0) / inputSize;
  const double sumOfXMinusMeanSquared =
      std::accumulate(inputValues.cbegin(), inputValues.cend(), 0.,
                      [mean](const auto sum, const auto x) {
                        return sum + std::pow(x - mean, 2);
                      });
  return sqrt(sumOfXMinusMeanSquared / (inputSize - 1));
}

std::vector<double>
standardDeviationArray(const std::vector<std::vector<double>> &yValues) {
  std::vector<double> standardDeviations;
  standardDeviations.reserve(yValues.size());
  std::transform(yValues.begin(), yValues.end(),
                 std::back_inserter(standardDeviations), standardDeviation);
  return standardDeviations;
}

/**
Get all histograms at a given index from a set of workspaces. Arranges the
output such that the first vector contains the first value from each workspace,
the second vector contains all the second values, etc.
*/
std::vector<std::vector<double>>
allYValuesAtIndex(const std::vector<MatrixWorkspace_sptr> &workspaces,
                  const std::size_t index) {
  std::vector<std::vector<double>> yValues(
      workspaces[0]->getDimension(0)->getNBins());
  for (auto &&workspace : workspaces) {
    const auto values = workspace->y(index).rawData();
    for (auto j = 0u; j < values.size(); ++j)
      yValues[j].emplace_back(values[j]);
  }
  return yValues;
}

int getWorkspaceNumberOfHistograms(MatrixWorkspace_sptr workspace) {
  return boost::numeric_cast<int>(workspace->getNumberHistograms());
}

} // namespace

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CalculateIqt)

const std::string CalculateIqt::name() const { return "CalculateIqt"; }

int CalculateIqt::version() const { return 1; }

const std::vector<std::string> CalculateIqt::seeAlso() const {
  return {"TransformToIqt"};
}

const std::string CalculateIqt::category() const {
  return "Inelastic\\Indirect";
}

const std::string CalculateIqt::summary() const {
  return "Calculates I(Q,t) from S(Q,w) and computes the errors using a "
         "monte-carlo routine.";
}

void CalculateIqt::init() {

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "The name of the sample workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("ResolutionWorkspace",
                                                        "", Direction::Input),
                  "The name of the resolution workspace.");

  declareProperty("EnergyMin", -0.5, "Minimum energy for fit. Default = -0.5.");
  declareProperty("EnergyMax", 0.5, "Maximum energy for fit. Default = 0.5.");
  declareProperty("EnergyWidth", 0.1, "Width of energy bins for fit.");

  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);

  declareProperty("NumberOfIterations", DEFAULT_ITERATIONS, positiveInt,
                  "Number of randomised simulations within error to run.");
  declareProperty(
      "SeedValue", DEFAULT_SEED, positiveInt,
      "Seed the random number generator for monte-carlo error calculation.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to use for the output workspace.");

  declareProperty("CalculateErrors", true, "Calculate monte-carlo errors.");
}

void CalculateIqt::exec() {
  const auto rebinParams = rebinParamsAsString();
  const MatrixWorkspace_sptr sampleWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr resolution = getProperty("ResolutionWorkspace");
  bool calculateErrors = getProperty("CalculateErrors");
  const int nIterations = getProperty("NumberOfIterations");
  const int seed = getProperty("SeedValue");
  resolution = normalizedFourierTransform(resolution, rebinParams);

  auto outputWorkspace =
      monteCarloErrorCalculation(sampleWorkspace, resolution, rebinParams, seed,
                                 calculateErrors, nIterations);

  outputWorkspace = removeInvalidData(outputWorkspace);
  setProperty("OutputWorkspace", outputWorkspace);
}

std::string CalculateIqt::rebinParamsAsString() {
  const double e_min = getProperty("EnergyMin");
  const double e_max = getProperty("EnergyMax");
  const double e_width = getProperty("EnergyWidth");
  return createRebinString(e_min, e_max, e_width);
}

MatrixWorkspace_sptr CalculateIqt::monteCarloErrorCalculation(
    MatrixWorkspace_sptr sample, MatrixWorkspace_sptr resolution,
    const std::string &rebinParams, const int seed, const bool calculateErrors,
    const int nIterations) {
  auto outputWorkspace = calculateIqt(sample, resolution, rebinParams);
  std::vector<MatrixWorkspace_sptr> simulatedWorkspaces;
  simulatedWorkspaces.reserve(nIterations);
  simulatedWorkspaces.emplace_back(outputWorkspace);

  MersenneTwister mTwister(seed);
  if (calculateErrors) {
    Progress errorCalculationProg(this, 0.0, 1.0, nIterations);
    PARALLEL_FOR_IF(Kernel::threadSafe(*sample, *resolution))
    for (auto i = 0; i < nIterations - 1; ++i) {
      errorCalculationProg.report("Calculating Monte Carlo errors...");
      PARALLEL_START_INTERUPT_REGION
      auto simulated =
          doSimulation(sample->clone(), resolution, rebinParams, mTwister);
      PARALLEL_CRITICAL(emplace_back)
      simulatedWorkspaces.emplace_back(simulated);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
    return setErrorsToStandardDeviation(simulatedWorkspaces);
  }
  return setErrorsToZero(simulatedWorkspaces);
}

std::map<std::string, std::string> CalculateIqt::validateInputs() {
  std::map<std::string, std::string> issues;
  const double eMin = getProperty("EnergyMin");
  const double eMax = getProperty("EnergyMax");
  if (eMin > eMax) {
    auto energy_swapped = "EnergyMin is greater than EnergyMax";
    issues["EnergyMin"] = energy_swapped;
    issues["EnergyMax"] = energy_swapped;
  }
  return issues;
}

MatrixWorkspace_sptr CalculateIqt::rebin(MatrixWorkspace_sptr workspace,
                                         const std::string &params) {
  IAlgorithm_sptr rebinAlgorithm = this->createChildAlgorithm("Rebin");
  rebinAlgorithm->initialize();
  rebinAlgorithm->setProperty("InputWorkspace", workspace);
  rebinAlgorithm->setProperty("OutputWorkspace", "_");
  rebinAlgorithm->setProperty("Params", params);
  rebinAlgorithm->execute();
  return rebinAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::integration(MatrixWorkspace_sptr workspace) {
  IAlgorithm_sptr integrationAlgorithm =
      this->createChildAlgorithm("Integration");
  integrationAlgorithm->initialize();
  integrationAlgorithm->setProperty("InputWorkspace", workspace);
  integrationAlgorithm->setProperty("OutputWorkspace", "_");
  integrationAlgorithm->execute();
  return integrationAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
CalculateIqt::convertToPointData(MatrixWorkspace_sptr workspace) {
  IAlgorithm_sptr pointDataAlgorithm =
      this->createChildAlgorithm("ConvertToPointData");
  pointDataAlgorithm->initialize();
  pointDataAlgorithm->setProperty("InputWorkspace", workspace);
  pointDataAlgorithm->setProperty("OutputWorkspace", "_");
  pointDataAlgorithm->execute();
  return pointDataAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
CalculateIqt::extractFFTSpectrum(MatrixWorkspace_sptr workspace) {
  IAlgorithm_sptr FFTAlgorithm =
      this->createChildAlgorithm("ExtractFFTSpectrum");
  FFTAlgorithm->initialize();
  FFTAlgorithm->setProperty("InputWorkspace", workspace);
  FFTAlgorithm->setProperty("OutputWorkspace", "_");
  FFTAlgorithm->setProperty("FFTPart", 2);
  FFTAlgorithm->execute();
  return FFTAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::divide(MatrixWorkspace_sptr lhsWorkspace,
                                          MatrixWorkspace_sptr rhsWorkspace) {
  IAlgorithm_sptr divideAlgorithm = this->createChildAlgorithm("Divide");
  divideAlgorithm->initialize();
  divideAlgorithm->setProperty("LHSWorkspace", lhsWorkspace);
  divideAlgorithm->setProperty("RHSWorkspace", rhsWorkspace);
  divideAlgorithm->setProperty("OutputWorkspace", "_");
  divideAlgorithm->execute();
  return divideAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::cropWorkspace(MatrixWorkspace_sptr workspace,
                                                 const double xMax) {
  IAlgorithm_sptr cropAlgorithm = this->createChildAlgorithm("CropWorkspace");
  cropAlgorithm->initialize();
  cropAlgorithm->setProperty("InputWorkspace", workspace);
  cropAlgorithm->setProperty("OutputWorkspace", "_");
  cropAlgorithm->setProperty("XMax", xMax);
  cropAlgorithm->execute();
  return cropAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
CalculateIqt::replaceSpecialValues(MatrixWorkspace_sptr workspace) {
  IAlgorithm_sptr specialValuesAlgorithm =
      this->createChildAlgorithm("ReplaceSpecialValues");
  specialValuesAlgorithm->initialize();
  specialValuesAlgorithm->setProperty("InputWorkspace", workspace);
  specialValuesAlgorithm->setProperty("OutputWorkspace", "_");
  specialValuesAlgorithm->setProperty("InfinityValue", 0.0);
  specialValuesAlgorithm->setProperty("BigNumberThreshold", 1.0001);
  specialValuesAlgorithm->setProperty("NaNValue", 0.0);
  specialValuesAlgorithm->execute();
  return specialValuesAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr
CalculateIqt::removeInvalidData(MatrixWorkspace_sptr workspace) {
  auto binning = (workspace->blocksize() + 1) / 2;
  auto binV = workspace->x(0)[binning];
  workspace = cropWorkspace(workspace, binV);
  return replaceSpecialValues(workspace);
}

MatrixWorkspace_sptr
CalculateIqt::normalizedFourierTransform(MatrixWorkspace_sptr workspace,
                                         const std::string &rebinParams) {
  workspace = rebin(workspace, rebinParams);
  auto workspace_int = integration(workspace);
  workspace = convertToPointData(workspace);
  workspace = extractFFTSpectrum(workspace);
  return divide(workspace, workspace_int);
}

MatrixWorkspace_sptr
CalculateIqt::calculateIqt(MatrixWorkspace_sptr workspace,
                           MatrixWorkspace_sptr resolutionWorkspace,
                           const std::string &rebinParams) {
  workspace = normalizedFourierTransform(workspace, rebinParams);
  return divide(workspace, resolutionWorkspace);
}

MatrixWorkspace_sptr CalculateIqt::doSimulation(MatrixWorkspace_sptr sample,
                                                MatrixWorkspace_sptr resolution,
                                                const std::string &rebinParams,
                                                MersenneTwister &mTwister) {
  auto simulatedWorkspace = randomizeWorkspaceWithinError(sample, mTwister);
  return calculateIqt(simulatedWorkspace, resolution, rebinParams);
}

MatrixWorkspace_sptr CalculateIqt::setErrorsToStandardDeviation(
    const std::vector<MatrixWorkspace_sptr> &simulatedWorkspaces) {
  auto outputWorkspace = simulatedWorkspaces.front();
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*outputWorkspace))
  for (auto i = 0; i < getWorkspaceNumberOfHistograms(outputWorkspace); ++i) {
    PARALLEL_START_INTERUPT_REGION
    outputWorkspace->mutableE(i) =
        standardDeviationArray(allYValuesAtIndex(simulatedWorkspaces, i));
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return outputWorkspace;
}

MatrixWorkspace_sptr CalculateIqt::setErrorsToZero(
    const std::vector<MatrixWorkspace_sptr> &simulatedWorkspaces) {
  auto outputWorkspace = simulatedWorkspaces.front();
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*outputWorkspace))
  for (auto i = 0; i < getWorkspaceNumberOfHistograms(outputWorkspace); ++i) {
    PARALLEL_START_INTERUPT_REGION
    outputWorkspace->mutableE(i) = 0;
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  return outputWorkspace;
}

} // namespace Algorithms
} // namespace Mantid
