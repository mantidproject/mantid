#include "MantidAlgorithms/CalMuonDetectorPhases.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalMuonDetectorPhases)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalMuonDetectorPhases::init() {
  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(
      new API::WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalMuonDetectorPhases::exec() {

  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  setProperty("OutputWorkspace",inputWS);
}

} // namespace Algorithms
} // namespace Mantid
