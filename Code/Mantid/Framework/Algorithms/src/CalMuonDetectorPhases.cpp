#include "MantidAlgorithms/CalMuonDetectorPhases.h"

#include "MantidAPI/ITableWorkspace.h"

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
      "Name of the reference input workspace");

  declareProperty("FirstGoodData", EMPTY_DBL(),
                  "The first good data point in units of "
                  "micro-seconds as measured from time "
                  "zero",
                  Direction::Input);

  declareProperty("LastGoodData", EMPTY_DBL(),
                  "The last good data point in units of "
                  "micro-seconds as measured from time "
                  "zero",
                  Direction::Input);

  declareProperty("Frequency", EMPTY_DBL(), "Starting hing for the frequency",
                  Direction::Input);

  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "DetectorTable", "", Direction::Output),
                  "The name of the TableWorkspace in which to store the list "
                  "of phases and asymmetries for each detector");

}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalMuonDetectorPhases::exec() {

  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  API::ITableWorkspace_sptr tab = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  setProperty("DetectorTable",tab);
}

} // namespace Algorithms
} // namespace Mantid
