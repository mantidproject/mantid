// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <iostream>
#include <random>

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

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InstrumentWorkspace", "", Direction::Input),
                    "Input Workspace containing data to be fitted");
  g_log.notice() << "False" << std::endl;
  declareProperty("Iterations", 100, mustBePositive, "The number of times the simulation will run.");
  declareProperty("Seed", 32, mustBePositive, "Integer that initializes a random-number generator");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMonteCarloWorkspace::exec() {
  MatrixWorkspace_sptr instWs = getProperty("InstrumentWorkspace");
  g_log.notice() << "True" << std::endl;
  int seed_input = getProperty("Seed");
  int num_iterations = getProperty("Iterations");
  MatrixWorkspace_sptr outputWs;

  const Mantid::HistogramData::Histogram &hist = instWs->histogram(0);
  const auto &yData = hist.y();

  // Used to store the random numbers picked
  std::vector<double> randomNum_list;
  randomNum_list.reserve(num_iterations);

  auto min = *std::min_element(yData.begin(), yData.end());
  auto max = *std::max_element(yData.begin(), yData.end());

  std::mt19937 gen(seed_input);
  std::uniform_real_distribution<> dis(min, max);

  // Repeat based on number of iterations given
  for (int i = 0; i < num_iterations; ++i) {
    double random_number = dis(gen); // Generate random number in range [min, max]
    randomNum_list.push_back(random_number);
    g_log.notice() << "Random number " << i + 1 << ": " << random_number << std::endl;

    // calculate the mean of random numbers
    // calculate variance of random numbers
    // plot normal dist using formula
  }

  // (plot expected_val - fitted_value) / fit_error
  setProperty("OutputWorkspace", outputWs);

  // check which bin random_number belongs to
  // place random_number it in respective bin

  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid




