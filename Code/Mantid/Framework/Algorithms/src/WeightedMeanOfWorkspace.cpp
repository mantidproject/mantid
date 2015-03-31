#include "MantidAlgorithms/WeightedMeanOfWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"

#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid {
using namespace API;
using namespace DataObjects;
using namespace Geometry;
using namespace Kernel;

namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(WeightedMeanOfWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
WeightedMeanOfWorkspace::WeightedMeanOfWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
WeightedMeanOfWorkspace::~WeightedMeanOfWorkspace() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string WeightedMeanOfWorkspace::name() const {
  return "WeightedMeanOfWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int WeightedMeanOfWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string WeightedMeanOfWorkspace::category() const {
  return "Arithmetic";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void WeightedMeanOfWorkspace::init() {
  this->declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  this->declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void WeightedMeanOfWorkspace::exec() {
  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  // Check if it is an event workspace
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventW != NULL) {
    throw std::runtime_error(
        "WeightedMeanOfWorkspace cannot handle EventWorkspaces!");
  }
  // Create the output workspace
  MatrixWorkspace_sptr singleValued =
      WorkspaceFactory::Instance().create("WorkspaceSingleValue", 1, 1, 1);
  // Calculate weighted mean
  std::size_t numHists = inputWS->getNumberHistograms();
  double averageValue = 0.0;
  double weightSum = 0.0;
  for (std::size_t i = 0; i < numHists; ++i) {
    try {
      IDetector_const_sptr det = inputWS->getDetector(i);
      if (det->isMonitor() || det->isMasked()) {
        continue;
      }
    } catch (...) {
      // Swallow these if no instrument is found
      ;
    }
    MantidVec y = inputWS->dataY(i);
    MantidVec e = inputWS->dataE(i);
    double weight = 0.0;
    for (std::size_t j = 0; j < y.size(); ++j) {
      if (!boost::math::isnan(y[j]) && !boost::math::isinf(y[j]) &&
          !boost::math::isnan(e[j]) && !boost::math::isinf(e[j])) {
        weight = 1.0 / (e[j] * e[j]);
        averageValue += (y[j] * weight);
        weightSum += weight;
      }
    }
  }
  singleValued->dataX(0)[0] = 0.0;
  singleValued->dataY(0)[0] = averageValue / weightSum;
  singleValued->dataE(0)[0] = std::sqrt(weightSum);
  this->setProperty("OutputWorkspace", singleValued);
}

} // namespace Algorithms
} // namespace Mantid
