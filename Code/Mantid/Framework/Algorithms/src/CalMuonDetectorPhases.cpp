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

  // Get the input ws
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // Get start and end time
  double startTime = getProperty("FirstGoodData");
  double endTime = getProperty("LastGoodData");

  // Get the frequency
  double freq = getProperty("Frequency");

  // Prepares the workspaces: extracts data from [startTime, endTime] and
  // removes exponential decay
  API::MatrixWorkspace_sptr tempWS = prepareWorkspace(inputWS, startTime, endTime);

  API::ITableWorkspace_sptr tab =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  setProperty("DetectorTable", tab);
}

/** TODO Description
*/
API::MatrixWorkspace_sptr
CalMuonDetectorPhases::prepareWorkspace(const API::MatrixWorkspace_sptr &ws,
                                        double startTime, double endTime) {

  // If startTime and/or endTime are EMPTY_DBL():
  if (startTime == EMPTY_DBL()) {
    // TODO set to zero for now, it should be read from FirstGoodBin
    startTime = 0.;
  }
  if (endTime == EMPTY_DBL()) {
    // Last available time
    endTime = ws->readX(0).back();
  }

  // Extract counts from startTime to endTime
  API::IAlgorithm_sptr crop = createChildAlgorithm("CropWorkspace");
  crop->setProperty("InputWorkspace",ws);
  crop->setProperty("XMin",startTime);
  crop->setProperty("XMax",endTime);
  crop->executeAsChildAlg();
  boost::shared_ptr<API::MatrixWorkspace> wsCrop =
      crop->getProperty("OutputWorkspace");

  // Remove exponential decay
  API::IAlgorithm_sptr remove = createChildAlgorithm("RemoveExpDecay");
  remove->setProperty("InputWorkspace",wsCrop);
  remove->executeAsChildAlg();
  boost::shared_ptr<API::MatrixWorkspace> wsRem =
      remove->getProperty("OutputWorkspace");

  return wsRem;

}

} // namespace Algorithms
} // namespace Mantid
