// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
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
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("CreateMonteCarloWorkspace");
}
namespace Mantid {
namespace Algorithms {
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramY;
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
                  "Input Workspace containing data to be fitted");
  declareProperty("Seed", 32, mustBePositive,
                  "Integer that initializes a random-number generator, good for reproducibility");
  declareProperty("MonteCarloEvents", 0, mustBePositive,
                  "Number of Monte Carlo events to simulate. Defaults to integral of input workspace if 0.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of output workspace.");
}

//----------------------------------------------------------------------------------------------

Mantid::HistogramData::HistogramY CreateMonteCarloWorkspace::fillHistogramWithRandomData(const std::vector<double> &cdf,
                                                                                         int numIterations,
                                                                                         int seed_input,
                                                                                         API::Progress &progress) {

  Mantid::HistogramData::HistogramY outputY(cdf.size(), 0.0);
  std::mt19937 gen(seed_input);
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

std::vector<double> CreateMonteCarloWorkspace::computeNormalizedCDF(const Mantid::HistogramData::HistogramY &yData) {
  std::vector<double> cdf(yData.size());
  std::partial_sum(yData.begin(), yData.end(), cdf.begin());
  double total_counts = cdf.back();
  // Normalize the CDF
  std::transform(cdf.begin(), cdf.end(), cdf.begin(), [total_counts](double val) { return val / total_counts; });
  return cdf;
}

int CreateMonteCarloWorkspace::computeNumberOfIterations(const Mantid::HistogramData::HistogramY &yData,
                                                         int userMCEvents) {
  if (userMCEvents > 0) {
    return userMCEvents;
  }

  // Default: Integral of Input
  double total_counts = std::accumulate(yData.begin(), yData.end(), 0.0);
  int iterations = static_cast<int>(std::round(total_counts));

  if (iterations == 0) {
    g_log.warning("Total counts in the input workspace round to 0. No Monte Carlo events will be generated.");
  }

  return iterations;
}

Mantid::HistogramData::HistogramY
CreateMonteCarloWorkspace::scaleInputToMatchMCEvents(const Mantid::HistogramData::HistogramY &yData,
                                                     int targetMCEvents) {

  double total_counts = std::accumulate(yData.begin(), yData.end(), 0.0);
  if (total_counts == 0) {
    g_log.warning("Total counts in the input workspace are 0. Scaling cannot be performed.");
    return yData;
  }

  double scaleFactor = static_cast<double>(targetMCEvents) / total_counts;

  Mantid::HistogramData::HistogramY scaledY(yData.size());
  std::transform(yData.begin(), yData.end(), scaledY.begin(),
                 [scaleFactor](double count) { return count * scaleFactor; });

  return scaledY;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */

// Using Cumulative Distribution Function
void CreateMonteCarloWorkspace::exec() {
  // Progress Bar set up
  int numSteps = 2;
  API::Progress progress(this, 0.0, 1.0, numSteps);

  MatrixWorkspace_sptr instWs = getProperty("InputWorkspace");
  int seed_input = getProperty("Seed");
  int userMCEvents = getProperty("MonteCarloEvents");

  const Mantid::HistogramData::HistogramY &originalYData = instWs->y(0); // Counts in each bin
  Mantid::HistogramData::HistogramY yData = originalYData;

  // Scale input workspace if user has specified MC events
  if (userMCEvents > 0) {
    g_log.warning() << "Custom Monte Carlo events: " << userMCEvents << ". Input workspace scaled accordingly.\n";
    yData = scaleInputToMatchMCEvents(originalYData, userMCEvents);
  }

  // Determine number of iterations
  int numIterations = computeNumberOfIterations(yData, userMCEvents);

  std::vector<double> cdf = computeNormalizedCDF(yData);
  progress.report("Computing normalized CDF...");

  MatrixWorkspace_sptr outputWs = WorkspaceFactory::Instance().create(instWs, 1);

  // Copy the bin boundaries (X-values) from the input to the output
  outputWs->setSharedX(0, instWs->sharedX(0));
  Mantid::HistogramData::HistogramY outputY = fillHistogramWithRandomData(cdf, numIterations, seed_input, progress);
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
