// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <cmath>
#include <random>
#include <string>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CreateBootstrapWorkspace.h"

#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Logger.h"

namespace {
Mantid::Kernel::Logger g_log("CreateBootstrapWorkspace");
}

namespace Mantid::Algorithms {
using Mantid::Kernel::Direction;
using namespace Mantid::API;
using namespace std;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateBootstrapWorkspace)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CreateBootstrapWorkspace::name() const { return "CreateBootstrapWorkspace"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateBootstrapWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateBootstrapWorkspace::category() const { return "Simulation"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateBootstrapWorkspace::summary() const {
  return "Creates a randomly simulated workspace by sampling from the probability distribution of input data.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateBootstrapWorkspace::init() {
  auto mustBePositive = std::make_shared<Mantid::Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input Workspace containing data to be simulated");
  declareProperty("Seed", 32, mustBePositive,
                  "Integer seed that initialises the random-number generator, for reproducibility");
  declareProperty("NumberOfReplicas", 0, mustBePositive,
                  "Number of Monte Carlo events to simulate. Defaults to integral of input workspace if 0.");
  declareProperty("useErrorSampling", true, "Whether to use sampling from errors");
  declareProperty("OutputPrefix", "", "Prefix to add to bootstrap workspaces");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspaceGroup", "bootstrap_samples",
                                                                      Direction::Output),
                  "Name of output workspace.");
}

//----------------------------------------------------------------------------------------------

HistogramData::HistogramY CreateBootstrapWorkspace::sampleHistogramFromGaussian(const HistogramData::HistogramY &dataY,
                                                                                const HistogramData::HistogramE &dataE,
                                                                                std::mt19937 &gen) {

  HistogramData::HistogramY outputY(dataY.size(), 0.0);

  for (size_t index = 0; index < dataY.size(); index++) {
    std::normal_distribution<double> dist(dataY[index], dataE[index]);
    outputY[index] = dist(gen);
  }
  return outputY;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateBootstrapWorkspace::exec() {

  auto &ADS = AnalysisDataService::Instance();
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  int inputSeed = getProperty("Seed");
  std::mt19937 gen(static_cast<size_t>(inputSeed));
  int numReplicas = getProperty("NumberOfReplicas");
  bool useErrorSampling = getProperty("useErrorSampling");
  std::string prefix = getProperty("OutputPrefix");
  Progress progress(this, 0, 1, numReplicas);

  std::vector<std::string> boot_names;

  for (int i = 1; i <= numReplicas; i++) {
    MatrixWorkspace_sptr bootWs = WorkspaceFactory::Instance().create(inputWs);
    ADS.addOrReplace(prefix + std::to_string(i), bootWs);
    boot_names.push_back(prefix + std::to_string(i));

    for (size_t index = 0; index < bootWs->getNumberHistograms(); index++) {
      bootWs->setSharedX(index, inputWs->sharedX(index));

      if (useErrorSampling) {
        bootWs->mutableY(index) = sampleHistogramFromGaussian(inputWs->y(index), inputWs->e(index), gen);
        bootWs->mutableE(index) = inputWs->e(index);

      } else {
        std::uniform_int_distribution<size_t> dist(0, inputWs->getNumberHistograms() - 1);
        size_t new_index = dist(gen);
        bootWs->mutableY(index) = inputWs->y(new_index);
        bootWs->mutableE(index) = inputWs->e(new_index);
      }
    }
    progress.report("Creating Bootstrap Samples...");
  }

  auto alg = createChildAlgorithm("GroupWorkspaces");
  alg->setProperty("InputWorkspaces", boot_names);
  alg->executeAsChildAlg();
  WorkspaceGroup_sptr outputGroup = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspaceGroup", outputGroup);
}
} // namespace Mantid::Algorithms
