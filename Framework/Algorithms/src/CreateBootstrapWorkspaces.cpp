// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <cmath>
#include <memory>
#include <random>
#include <string>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/CreateBootstrapWorkspaces.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/normal_distribution.h"
#include "MantidKernel/uniform_int_distribution.h"

using Mantid::Kernel::BoundedValidator;
using Mantid::Kernel::Direction;
using Mantid::Kernel::StringListValidator;
using namespace Mantid::API;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateBootstrapWorkspaces)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CreateBootstrapWorkspaces::name() const { return "CreateBootstrapWorkspaces"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateBootstrapWorkspaces::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateBootstrapWorkspaces::category() const { return "Simulation"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateBootstrapWorkspaces::summary() const {
  return "Creates a randomly simulated workspace by sampling from the input data.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateBootstrapWorkspaces::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input Workspace containing data to be simulated");
  declareProperty("Seed", 32, "Integer seed that initialises the random-number generator, for reproducibility");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty("NumberOfReplicas", 100, mustBePositive, "Number of Bootstrap workspaces to simulate.");

  std::vector<std::string> bootstrapOptions{"ErrorSampling", "SpectraSampling"};
  declareProperty(
      "BootstrapType", "ErrorSampling", std::make_shared<StringListValidator>(bootstrapOptions),
      "Type of bootstrap sampling to use. "
      "ErrorSampling samples at each data point, while SpectraSampling samples each spectra with repetition.");

  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspaceGroup", "bootstrap", Direction::Output),
      "Name of output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateBootstrapWorkspaces::exec() {

  auto &ADS = AnalysisDataService::Instance();
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  int inputSeed = getProperty("Seed");
  std::mt19937 gen(static_cast<unsigned int>(inputSeed));

  int numReplicas = getProperty("NumberOfReplicas");
  std::string bootType = getProperty("BootstrapType");
  std::string prefix = getProperty("OutputWorkspaceGroup");
  Progress progress(this, 0, 1, numReplicas);

  std::vector<std::string> bootNames;

  for (int i = 1; i <= numReplicas; i++) {
    MatrixWorkspace_sptr bootWs = WorkspaceFactory::Instance().create(inputWs);
    std::string wsName = prefix + '_' + std::to_string(i);
    ADS.addOrReplace(wsName, bootWs);
    bootNames.push_back(std::move(wsName));

    for (size_t index = 0; index < bootWs->getNumberHistograms(); index++) {
      bootWs->setSharedX(index, inputWs->sharedX(index));

      if (bootType == "ErrorSampling") {
        bootWs->mutableY(index) = sampleHistogramFromGaussian(inputWs->y(index), inputWs->e(index), gen);
        bootWs->mutableE(index) = 0;

      } else if (bootType == "SpectraSampling") {
        // Sample from spectra indices with replacement
        Kernel::uniform_int_distribution<size_t> dist(0, inputWs->getNumberHistograms() - 1);
        size_t randomIndex = dist(gen);
        bootWs->mutableY(index) = inputWs->y(randomIndex);
        bootWs->mutableE(index) = inputWs->e(randomIndex);
      }
    }
    progress.report("Creating Bootstrap Samples...");
  }
  auto alg = createChildAlgorithm("GroupWorkspaces");
  alg->setProperty("InputWorkspaces", bootNames);
  alg->executeAsChildAlg();
  WorkspaceGroup_sptr outputGroup = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspaceGroup", outputGroup);
}

//----------------------------------------------------------------------------------------------
/** Helpers.
 */
HistogramData::HistogramY CreateBootstrapWorkspaces::sampleHistogramFromGaussian(const HistogramData::HistogramY &dataY,
                                                                                 const HistogramData::HistogramE &dataE,
                                                                                 std::mt19937 &gen) {
  HistogramData::HistogramY outputY(dataY.size(), 0.0);
  // For each bin, sample y from Gaussian(y, e)
  for (size_t index = 0; index < dataY.size(); index++) {
    Kernel::normal_distribution<double> dist(dataY[index], dataE[index]);
    outputY[index] = dist(gen);
  }
  return outputY;
}
} // namespace Mantid::Algorithms
