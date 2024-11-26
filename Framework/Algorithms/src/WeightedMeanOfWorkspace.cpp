// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/WeightedMeanOfWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
using namespace API;
using namespace DataObjects;
using namespace Geometry;
using namespace Kernel;

namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(WeightedMeanOfWorkspace)

/// Algorithm's name for identification. @see Algorithm::name
const std::string WeightedMeanOfWorkspace::name() const { return "WeightedMeanOfWorkspace"; }

/// Algorithm's version for identification. @see Algorithm::version
int WeightedMeanOfWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string WeightedMeanOfWorkspace::category() const { return "Arithmetic"; }

/** Initialize the algorithm's properties.
 */
void WeightedMeanOfWorkspace::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                        "An input workspace.");
  this->declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                        "An output workspace.");
}

/** Execute the algorithm.
 */
void WeightedMeanOfWorkspace::exec() {
  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  // Check if it is an event workspace
  EventWorkspace_const_sptr eventW = std::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != nullptr) {
    throw std::runtime_error("WeightedMeanOfWorkspace cannot handle EventWorkspaces!");
  }
  // Create the output workspace
  MatrixWorkspace_sptr singleValued = WorkspaceFactory::Instance().create("WorkspaceSingleValue", 1, 1, 1);
  // Calculate weighted mean
  std::size_t numHists = inputWS->getNumberHistograms();
  double averageValue = 0.0;
  double weightSum = 0.0;
  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (std::size_t i = 0; i < numHists; ++i) {
    if (spectrumInfo.hasDetectors(i))
      if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
        continue;
    auto &y = inputWS->y(i);
    auto &e = inputWS->e(i);
    for (std::size_t j = 0; j < y.size(); ++j) {
      if (std::isfinite(y[j]) && std::isfinite(e[j])) {
        double weight = 1.0 / (e[j] * e[j]);
        averageValue += (y[j] * weight);
        weightSum += weight;
      }
    }
  }
  singleValued->mutableX(0)[0] = 0.0;
  singleValued->mutableY(0)[0] = averageValue / weightSum;
  singleValued->mutableE(0)[0] = std::sqrt(weightSum);
  this->setProperty("OutputWorkspace", singleValued);
}

} // namespace Algorithms
} // namespace Mantid
