// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <iostream>
#include <random>
#include <numeric> // For std::accumulate
#include <vector>
#include <algorithm>

#include "MantidAlgorithms/CreateMonteCarloWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/WorkspaceFactory.h"


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
  declareProperty("Iterations", 100, mustBePositive, "The number of times the simulation will run.");
  declareProperty("Seed", 32, mustBePositive, "Integer that initializes a random-number generator");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */

// Calculate the mean of a vector of integers
double CreateMonteCarloWorkspace::calculateMean(const std::vector<int> &numbers) {
  if (numbers.empty()) {
    return 0.0; // Avoid division by zero
  }
  int sum = std::accumulate(numbers.begin(), numbers.end(), 0);
  return static_cast<double>(sum) / numbers.size(); // Cast to double for the mean
}

// Calculate the variance of a vector of integers
double CreateMonteCarloWorkspace::calculateVariance(const std::vector<int> &numbers) {
  if (numbers.size() < 2) {
    return 0.0; // Variance is zero for a single number or an empty list
  }

  // Step 1: Calculate the mean
  double mean = calculateMean(numbers);

  // Step 2: Calculate the squared differences from the mean
  double variance_sum = 0.0;
  for (int num : numbers) {
    variance_sum += (num - mean) * (num - mean); // (x_i - mean)^2
  }

  // Step 3: Divide by the number of elements
  return variance_sum / numbers.size();
}


void CreateMonteCarloWorkspace::exec() {
  MatrixWorkspace_sptr instWs = getProperty("InputWorkspace");
  int seed_input = getProperty("Seed");
  int num_iterations = getProperty("Iterations");
  MatrixWorkspace_sptr outputWs;

  // Create an output workspace with one histogram and 'num_iterations' bins
  outputWs = WorkspaceFactory::Instance().create(instWs, 1, num_iterations + 1, num_iterations);
  const Mantid::HistogramData::Histogram &hist = instWs->histogram(0);
  const auto &yData = hist.y();

  // Used to store the random numbers picked
  std::vector<int> randomNumList;
  randomNumList.reserve(num_iterations);

  auto min = static_cast<int>(*std::min_element(yData.begin(), yData.end()));
  auto max = static_cast<int>(*std::max_element(yData.begin(), yData.end()));

  std::mt19937 gen(seed_input);
  std::uniform_int_distribution<> dis(min, max);

  // Repeat based on number of iterations given
  for (int i = 0; i < num_iterations; ++i) {
    int random_number = dis(gen); // Generate random number in range [min, max]
    randomNumList.push_back(random_number);
    g_log.notice() << "Random number " << i + 1 << ": " << random_number << std::endl;

    // calculate variance of random numbers
    // plot normal dist using formula

    // Fill output workspace
    outputWs->mutableX(0)[i] = static_cast<double>(i); // iteration number
    outputWs->mutableY(0)[i] = random_number;          // random number
  }

  // calculate the mean of random numbers
  double avg = calculateMean(randomNumList);
  g_log.notice() << "Average: " << avg << std::endl;

  double variance = calculateVariance(randomNumList);
  g_log.notice() << "Variance: " << variance << std::endl;


  // (plot expected_val - fitted_value) / fit_error
  if (num_iterations < outputWs->mutableX(0).size()) {
    outputWs->mutableX(0)[num_iterations] = static_cast<double>(num_iterations);
  } else {
    g_log.error() << "Index out of bounds in setting X axis." << std::endl;
  }
  setProperty("OutputWorkspace", outputWs);

  // check which bin random_number belongs to
  // place random_number it in respective bin

  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid




