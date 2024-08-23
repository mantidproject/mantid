// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include <utility>

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
void randomizeHistogramWithinError(HistogramY &row, const HistogramE &errors, Generator &generator) {
  for (auto i = 0u; i < row.size(); ++i)
    row[i] += generator(errors[i]);
}

MatrixWorkspace_sptr randomizeWorkspaceWithinError(MatrixWorkspace_sptr workspace, MersenneTwister &mTwister) {
  auto randomNumberGenerator = [&mTwister](const double error) { return mTwister.nextValue(-error, error); };
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
    randomizeHistogramWithinError(workspace->mutableY(i), workspace->e(i), randomNumberGenerator);
  return workspace;
}

double standardDeviation(const std::vector<double> &inputValues) {
  const auto inputSize = boost::numeric_cast<double>(inputValues.size());
  const auto mean = std::accumulate(inputValues.begin(), inputValues.end(), 0.0) / inputSize;
  const double sumOfXMinusMeanSquared =
      std::accumulate(inputValues.cbegin(), inputValues.cend(), 0.,
                      [mean](const auto sum, const auto x) { return sum + std::pow(x - mean, 2); });
  return sqrt(sumOfXMinusMeanSquared / (inputSize - 1));
}

std::vector<double> standardDeviationArray(const std::vector<std::vector<double>> &yValues) {
  std::vector<double> standardDeviations;
  standardDeviations.reserve(yValues.size());
  std::transform(yValues.begin(), yValues.end(), std::back_inserter(standardDeviations), standardDeviation);
  return standardDeviations;
}

/**
Get all histograms at a given index from a set of workspaces. Arranges the
output such that the first vector contains the first value from each workspace,
the second vector contains all the second values, etc.
*/
std::vector<std::vector<double>> allYValuesAtIndex(const std::vector<MatrixWorkspace_sptr> &workspaces,
                                                   const std::size_t index) {
  std::vector<std::vector<double>> yValues(workspaces[0]->getDimension(0)->getNBins());
  for (auto &&workspace : workspaces) {
    const auto values = workspace->y(index).rawData();
    for (auto j = 0u; j < values.size(); ++j)
      yValues[j].emplace_back(values[j]);
  }
  return yValues;
}

int getWorkspaceNumberOfHistograms(const MatrixWorkspace_sptr &workspace) {
  return boost::numeric_cast<int>(workspace->getNumberHistograms());
}

} // namespace

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(CalculateIqt)

const std::string CalculateIqt::name() const { return "CalculateIqt"; }

int CalculateIqt::version() const { return 1; }

const std::vector<std::string> CalculateIqt::seeAlso() const { return {"TransformToIqt"}; }

const std::string CalculateIqt::category() const { return "Inelastic\\Indirect"; }

const std::string CalculateIqt::summary() const {
  return "Calculates I(Q,t) from S(Q,w) and computes the errors using a "
         "monte-carlo routine.";
}

void CalculateIqt::init() {

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "The name of the sample workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("ResolutionWorkspace", "", Direction::Input),
                  "The name of the resolution workspace.");

  declareProperty("EnergyMin", -0.5, "Minimum energy for fit. Default = -0.5.");
  declareProperty("EnergyMax", 0.5, "Maximum energy for fit. Default = 0.5.");
  declareProperty("EnergyWidth", 0.1, "Width of energy bins for fit.");

  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);

  declareProperty("NumberOfIterations", DEFAULT_ITERATIONS, positiveInt,
                  "Number of randomised simulations within error to run.");
  declareProperty("SeedValue", DEFAULT_SEED, positiveInt,
                  "Seed the random number generator for monte-carlo error calculation.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace.");

  declareProperty("CalculateErrors", true, "Calculate monte-carlo errors.");

  // option for enforcing the "Divide" operation for SampleWorkspace and ResolutionWorkspace at the last step
  // default is True
  declareProperty("EnforceNormalization", true, "Normalization to enforce I(t=0)");
}

void CalculateIqt::exec() {
  const auto rebinParams = rebinParamsAsString();
  const MatrixWorkspace_sptr sampleWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr resolution = getProperty("ResolutionWorkspace");
  bool calculateErrors = getProperty("CalculateErrors");
  bool enforceNormalization = getProperty("EnforceNormalization");
  const int nIterations = getProperty("NumberOfIterations");
  const int seed = getProperty("SeedValue");

  m_sampleIntegral = integration(rebin(sampleWorkspace, rebinParams));
  m_resolutionIntegral = integration(rebin(resolution, rebinParams));

  resolution = fourierTransform(resolution, rebinParams);
  if (enforceNormalization) {
    resolution = divide(resolution, m_resolutionIntegral);
  }

  auto outputWorkspace = monteCarloErrorCalculation(sampleWorkspace, resolution, rebinParams, seed, calculateErrors,
                                                    nIterations, enforceNormalization);

  outputWorkspace = replaceSpecialValues(outputWorkspace);
  setProperty("OutputWorkspace", outputWorkspace);
}

std::string CalculateIqt::rebinParamsAsString() {
  const double e_min = getProperty("EnergyMin");
  const double e_max = getProperty("EnergyMax");
  const double e_width = getProperty("EnergyWidth");
  return createRebinString(e_min, e_max, e_width);
}

MatrixWorkspace_sptr CalculateIqt::monteCarloErrorCalculation(const MatrixWorkspace_sptr &sample,
                                                              const MatrixWorkspace_sptr &resolution,
                                                              const std::string &rebinParams, const int seed,
                                                              const bool calculateErrors, const int nIterations,
                                                              const bool enforceNormalization) {
  auto outputWorkspace = calculateIqt(sample, resolution, rebinParams, enforceNormalization);
  std::vector<MatrixWorkspace_sptr> simulatedWorkspaces;
  simulatedWorkspaces.reserve(nIterations);
  simulatedWorkspaces.emplace_back(outputWorkspace);

  MersenneTwister mTwister(seed);
  if (calculateErrors) {
    Progress errorCalculationProg(this, 0.0, 1.0, nIterations);
    PARALLEL_FOR_IF(Kernel::threadSafe(*sample, *resolution))
    for (auto i = 0; i < nIterations - 1; ++i) {
      errorCalculationProg.report("Calculating Monte Carlo errors...");
      PARALLEL_START_INTERRUPT_REGION
      auto simulated = doSimulation(sample->clone(), resolution, rebinParams, mTwister, enforceNormalization);
      PARALLEL_CRITICAL(emplace_back)
      simulatedWorkspaces.emplace_back(simulated);
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
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

MatrixWorkspace_sptr CalculateIqt::rebin(const MatrixWorkspace_sptr &workspace, const std::string &params) {
  auto rebinAlgorithm = createChildAlgorithm("Rebin");
  rebinAlgorithm->initialize();
  rebinAlgorithm->setProperty("InputWorkspace", workspace);
  rebinAlgorithm->setProperty("OutputWorkspace", "_");
  rebinAlgorithm->setProperty("Params", params);
  rebinAlgorithm->execute();
  return rebinAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::integration(const MatrixWorkspace_sptr &workspace) {
  auto integrationAlgorithm = createChildAlgorithm("Integration");
  integrationAlgorithm->initialize();
  integrationAlgorithm->setProperty("InputWorkspace", workspace);
  integrationAlgorithm->setProperty("OutputWorkspace", "_");
  integrationAlgorithm->execute();
  return integrationAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::convertToPointData(const MatrixWorkspace_sptr &workspace) {
  auto pointDataAlgorithm = createChildAlgorithm("ConvertToPointData");
  pointDataAlgorithm->initialize();
  pointDataAlgorithm->setProperty("InputWorkspace", workspace);
  pointDataAlgorithm->setProperty("OutputWorkspace", "_");
  pointDataAlgorithm->execute();
  return pointDataAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::extractFFTSpectrum(const MatrixWorkspace_sptr &workspace) {
  auto FFTAlgorithm = createChildAlgorithm("ExtractFFTSpectrum");
  FFTAlgorithm->initialize();
  FFTAlgorithm->setProperty("InputWorkspace", workspace);
  FFTAlgorithm->setProperty("OutputWorkspace", "_");
  FFTAlgorithm->setProperty("FFTPart", 2);
  FFTAlgorithm->execute();
  return FFTAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::divide(const MatrixWorkspace_sptr &lhsWorkspace,
                                          const MatrixWorkspace_sptr &rhsWorkspace) {
  auto divideAlgorithm = createChildAlgorithm("Divide");
  divideAlgorithm->initialize();
  divideAlgorithm->setProperty("LHSWorkspace", lhsWorkspace);
  divideAlgorithm->setProperty("RHSWorkspace", rhsWorkspace);
  divideAlgorithm->setProperty("OutputWorkspace", "_");
  divideAlgorithm->execute();
  return divideAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::cropWorkspace(const MatrixWorkspace_sptr &workspace, const double xMax) {
  auto cropAlgorithm = createChildAlgorithm("CropWorkspace");
  cropAlgorithm->initialize();
  cropAlgorithm->setProperty("InputWorkspace", workspace);
  cropAlgorithm->setProperty("OutputWorkspace", "_");
  cropAlgorithm->setProperty("XMax", xMax);
  cropAlgorithm->execute();
  return cropAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::replaceSpecialValues(const MatrixWorkspace_sptr &workspace) {
  auto specialValuesAlgorithm = createChildAlgorithm("ReplaceSpecialValues");
  specialValuesAlgorithm->initialize();
  specialValuesAlgorithm->setProperty("InputWorkspace", workspace);
  specialValuesAlgorithm->setProperty("OutputWorkspace", "_");
  specialValuesAlgorithm->setProperty("InfinityValue", 0.0);
  specialValuesAlgorithm->setProperty("BigNumberThreshold", 1.0001);
  specialValuesAlgorithm->setProperty("NaNValue", 0.0);
  specialValuesAlgorithm->execute();
  return specialValuesAlgorithm->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr CalculateIqt::fourierTransform(MatrixWorkspace_sptr workspace, const std::string &rebinParams) {
  workspace = rebin(workspace, rebinParams);
  workspace = convertToPointData(workspace);
  workspace = extractFFTSpectrum(workspace);
  return workspace;
}

MatrixWorkspace_sptr CalculateIqt::calculateIqt(MatrixWorkspace_sptr workspace,
                                                const MatrixWorkspace_sptr &resolutionWorkspace,
                                                const std::string &rebinParams, const bool enforceNormalization) {
  workspace = fourierTransform(workspace, rebinParams);
  if (enforceNormalization) {
    workspace = divide(workspace, m_sampleIntegral);
  }
  return divide(workspace, resolutionWorkspace);
}

MatrixWorkspace_sptr CalculateIqt::doSimulation(MatrixWorkspace_sptr sample, const MatrixWorkspace_sptr &resolution,
                                                const std::string &rebinParams, MersenneTwister &mTwister,
                                                const bool enforceNormalization) {
  auto simulatedWorkspace = randomizeWorkspaceWithinError(std::move(sample), mTwister);
  return calculateIqt(simulatedWorkspace, resolution, rebinParams, enforceNormalization);
}

MatrixWorkspace_sptr
CalculateIqt::setErrorsToStandardDeviation(const std::vector<MatrixWorkspace_sptr> &simulatedWorkspaces) {
  auto outputWorkspace = simulatedWorkspaces.front();
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*outputWorkspace))
  for (auto i = 0; i < getWorkspaceNumberOfHistograms(outputWorkspace); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    outputWorkspace->mutableE(i) = standardDeviationArray(allYValuesAtIndex(simulatedWorkspaces, i));
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  return outputWorkspace;
}

MatrixWorkspace_sptr CalculateIqt::setErrorsToZero(const std::vector<MatrixWorkspace_sptr> &simulatedWorkspaces) {
  auto outputWorkspace = simulatedWorkspaces.front();
  PARALLEL_FOR_IF(Mantid::Kernel::threadSafe(*outputWorkspace))
  for (auto i = 0; i < getWorkspaceNumberOfHistograms(outputWorkspace); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    outputWorkspace->mutableE(i) = 0;
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  return outputWorkspace;
}

} // namespace Mantid::Algorithms
