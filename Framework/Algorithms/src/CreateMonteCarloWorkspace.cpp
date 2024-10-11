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


namespace Mantid {
namespace Algorithms {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

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

  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("Iterations", "", Direction::Input),
                  "The number of times the simulation will run.");
  declareProperty("Seed", 32, mustBePositive, "Integer that initializes a random-number generator");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of output workspace.");
  // Prompt Data input (workspace)
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMonteCarloWorkspace::exec() {
  int NumOfSim = 0;
  int seed_input = getProperty("Seed");
  int num_iterations = getProperty("Iterations");
  // get workspace input

  // Initialize the random number generator with the user-provided seed
  std::mt19937 gen(seed_input);

  // Repeat based on number of iterations given

  // calculate the mean of the histogram

  int min = 0;
  int max = 5; // Set this to n in the future

  std::uniform_int_distribution<int> dis(min, max);
  int random_number = dis(gen); // Make sure it generates same num based on given seed
  std::cout << "Random number: " << random_number << std::endl;

  // check which bin random_number belongs to
  // place random_number it in respective bin

  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
