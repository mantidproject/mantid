// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <numeric> // For std::accumulate
#include <random>
#include <vector>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CreateMonteCarloWorkspace.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("CreateMonteCarloWorkspace");
}
namespace Mantid {
namespace Algorithms {
using Mantid::Kernel::Direction;
using namespace Mantid::API;
using namespace std;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateMonteCarloWorkspace)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CreateMonteCarloWorkspace::name() const { return "CreateMonteCarloWorkspace"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateMonteCarloWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateMonteCarloWorkspace::category() const { return "Simulation"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateMonteCarloWorkspace::summary() const {
  return "Creates a randomly simulated workspace by sampling from the probability distribution of input data.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMonteCarloWorkspace::init() {
  auto mustBePositive = std::make_shared<Mantid::Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input Workspace containing data to be simulated");
  declareProperty("Seed", 32, mustBePositive,
                  "Integer seed that initialises the random-number generator, for reproducibility");
  declareProperty("MonteCarloEvents", 0, mustBePositive,
                  "Number of Monte Carlo events to simulate. Defaults to integral of input workspace if 0.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of output workspace.");
}

//----------------------------------------------------------------------------------------------

Mantid::HistogramData::HistogramY CreateMonteCarloWorkspace::fillHistogramWithRandomData(const std::vector<double> &cdf,
                                                                                         int numIterations,
                                                                                         int seedInput,
                                                                                         API::Progress &progress) {

  Mantid::HistogramData::HistogramY outputY(cdf.size(), 0.0);
  std::mt19937 gen(seedInput);
  std::uniform_real_distribution<> dis(0.0, 1.0);

  int progressInterval = std::max(1, numIterations / 100); // Update progress every 1%

  for (int i = 0; i < numIterations; ++i) {
    double randomNum = dis(gen);
    auto it = std::lower_bound(cdf.begin(), cdf.end(), randomNum);
    size_t index = std::distance(cdf.begin(), it);

    if (index < outputY.size()) {
      outputY[index] += 1.0;
    }

    if (i % progressInterval == 0) {
      progress.report("Generating random data...");
    }
  }
  return outputY;
}

/**
 *  Compute a normalized CDF [0..1] from the given histogram data.
 */
std::vector<double> CreateMonteCarloWorkspace::computeNormalizedCDF(const Mantid::HistogramData::HistogramY &yData) {
  std::vector<double> cdf(yData.size());
  std::partial_sum(yData.begin(), yData.end(), cdf.begin());
  double totalCounts = cdf.back();

  if (totalCounts > 0.0) {
    // Normalize the CDF
    std::transform(cdf.begin(), cdf.end(), cdf.begin(), [totalCounts](double val) { return val / totalCounts; });
  } else {
    g_log.warning("Total counts are zero; normalization skipped.");
  }
  return cdf;
}

/**
 *  Determine how many iterations to use for MC sampling.
 *  If userMCEvents > 0, use that directly; otherwise use the integral of the input data.
 */
int CreateMonteCarloWorkspace::integrateYData(const Mantid::HistogramData::HistogramY &yData) {
  double totalCounts = std::accumulate(yData.begin(), yData.end(), 0.0);
  int iterations = static_cast<int>(std::round(totalCounts));

  if (iterations == 0) {
    g_log.warning("Total counts in the input workspace round to 0. No Monte Carlo events will be generated.");
  }
  return iterations;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMonteCarloWorkspace::exec() {
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  int seedInput = getProperty("Seed");
  int userMCEvents = getProperty("MonteCarloEvents");

  const auto &originalYData = inputWs->y(0); // Counts in each bin

  int numIterations = (userMCEvents > 0) ? userMCEvents : integrateYData(originalYData);

  API::Progress progress(this, 0.0, 1.0, 101);
  progress.report("Computing normalized CDF...");
  std::vector<double> cdf = computeNormalizedCDF(originalYData);

  MatrixWorkspace_sptr outputWs = WorkspaceFactory::Instance().create(inputWs, 1);
  outputWs->setSharedX(0, inputWs->sharedX(0));

  // Fill the bins with random data, following the distribution in the CDF
  Mantid::HistogramData::HistogramY outputY = fillHistogramWithRandomData(cdf, numIterations, seedInput, progress);
  outputWs->mutableY(0) = outputY;

  // Calculate errors as the square root of the counts
  Mantid::HistogramData::HistogramE outputE(outputY.size());
  std::transform(outputY.begin(), outputY.end(), outputE.begin(), [](double count) { return std::sqrt(count); });
  outputWs->mutableE(0) = outputE;

  g_log.warning("Only the first spectrum is being plotted.");

  setProperty("OutputWorkspace", outputWs);
}

} // namespace Algorithms
} // namespace Mantid
