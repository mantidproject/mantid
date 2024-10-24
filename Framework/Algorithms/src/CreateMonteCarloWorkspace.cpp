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
const std::string CreateMonteCarloWorkspace::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateMonteCarloWorkspace::summary() const { return "TODO: FILL IN A SUMMARY"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMonteCarloWorkspace::init() {
  auto mustBePositive = std::make_shared<Mantid::Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input Workspace containing data to be fitted");
  declareProperty("Iterations", 1000, mustBePositive, "The number of times the simulation will run.");
  declareProperty("Seed", 32, mustBePositive, "Integer that initializes a random-number generator");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of output workspace.");
}

void CreateMonteCarloWorkspace::afterPropertySet(const std::string &name) {
  if (name == "InputWorkspace") {
    g_log.notice() << "Name:" << name << endl;
    MatrixWorkspace_sptr instWs = getProperty("InputWorkspace");
    if (instWs) {
      // Get the number of data points (y-values) in the input workspace
      const Mantid::HistogramData::Histogram &hist = instWs->histogram(0);
      const auto &yData = hist.y();
      int numDataPoints = static_cast<int>(yData.size());

      setProperty("Iterations", numDataPoints);
      g_log.notice() << "Iterations:" << numDataPoints << endl;
    }
  }
}

//----------------------------------------------------------------------------------------------

// Calculate the mean of a vector of integers
double CreateMonteCarloWorkspace::calculateMean(const vector<int> &numbers) {
  if (numbers.empty()) {
    return 0.0; // Avoid division by zero
  }
  int sum = accumulate(numbers.cbegin(), numbers.cend(), 0);
  return static_cast<double>(sum) / numbers.size(); // Cast to double for the mean
}

//----------------------------------------------------------------------------------------------

// Calculate the variance of a vector of integers
double CreateMonteCarloWorkspace::calculateStandardDeviation(const vector<int> &numbers, double mean) {
  if (numbers.size() < 2) {
    return 0.0; // Standard Deviation is zero for a single number or an empty list
  }

  // Step 1: Calculate the squared differences from the mean
  double StandardDeviation_sum =
      accumulate(numbers.cbegin(), numbers.cend(), 0.0,
                 [&mean](double total, const double number) { return total + pow((number - mean), 2); });

  // Step 2: Divide by the number of elements
  return sqrt(StandardDeviation_sum / numbers.size() - 1);
}

//----------------------------------------------------------------------------------------------

// Probability Density Function
vector<double> CreateMonteCarloWorkspace::pdf(const vector<int> &numbers, double mean, double variance) const {
  double base = 1.0 / (sqrt(variance) * sqrt(2 * numbers::pi));
  double exponent = 0.0;

  vector<double> result;

  for (int number : numbers) {
    exponent = exp(-((number - mean) * (number - mean)) / (2 * variance));
    double pdf_value = base * exponent; // PDF for the current value
    result.push_back(pdf_value);
  }

  return result;
}

//----------------------------------------------------------------------------------------------

// Kernel Density Estimation function
vector<double> CreateMonteCarloWorkspace::kde(const vector<int> &numbers, double bandwidth) const {
  vector<double> result(numbers.size(), 0.0);
  size_t n = numbers.size();

  for (size_t i = 0; i < n; ++i) {
    double sum = 0.0;
    for (size_t j = 0; j < n; ++j) {
      double u = (numbers[i] - numbers[j]) / bandwidth;
      sum += exp(-0.5 * u * u) / (sqrt(2 * std::numbers::pi));
    }
    result[i] = sum / (n * bandwidth);
  }

  return result;
}

//----------------------------------------------------------------------------------------------

double CreateMonteCarloWorkspace::calculateBandwidth(const vector<int> &numbers) {
  double std_dev = calculateStandardDeviation(numbers, calculateMean(numbers));
  size_t n = numbers.size();
  return 1.06 * std_dev * pow(n, -0.2); // Silverman's Rule
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */

// Using Cumulative Distribution Function
void CreateMonteCarloWorkspace::exec() {
  MatrixWorkspace_sptr instWs = getProperty("InputWorkspace");
  int seed_input = getProperty("Seed");

  const Mantid::HistogramData::Histogram &hist = instWs->histogram(0);
  const auto &xData = hist.x(); // Bin boundaries
  const auto &yData = hist.y(); // Data in each bin

  double total_counts = std::accumulate(yData.begin(), yData.end(), 0.0);

  int numIterations = static_cast<int>(std::round(total_counts));

  std::vector<double> cdf(yData.size());
  std::partial_sum(yData.begin(), yData.end(), cdf.begin());

  // Normalize the CDF
  std::transform(cdf.begin(), cdf.end(), cdf.begin(), [total_counts](double val) { return val / total_counts; });

  MatrixWorkspace_sptr outputWs = WorkspaceFactory::Instance().create(instWs);

  // Copy the bin boundaries from the input to the output
  outputWs->setSharedX(0, hist.sharedX());

  auto &outputY = outputWs->mutableY(0);
  outputY.assign(outputY.size(), 0.0);

  std::mt19937 gen(seed_input);
  std::uniform_real_distribution<> dis(0.0, 1.0);

  for (int i = 0; i < numIterations; ++i) {
    double randomNumList = dis(gen);

    // Find the bin corresponding to the random number in the CDF
    auto it = std::lower_bound(cdf.begin(), cdf.end(), randomNumList);
    size_t index = std::distance(cdf.begin(), it);

    // Ensure the index is within bounds
    if (index < outputY.size()) {
      outputY[index] += 1.0;
    }
  }

  setProperty("OutputWorkspace", outputWs);
}

} // namespace Algorithms
} // namespace Mantid
