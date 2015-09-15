#include "MantidDataHandling/ExtractMonitorWorkspace.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractMonitorWorkspace)

using namespace Mantid::Kernel;
using namespace Mantid::API;

ExtractMonitorWorkspace::ExtractMonitorWorkspace() {}

ExtractMonitorWorkspace::~ExtractMonitorWorkspace() {}

/// Algorithm's name for identification. @see Algorithm::name
const std::string ExtractMonitorWorkspace::name() const {
  return "ExtractMonitorWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int ExtractMonitorWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractMonitorWorkspace::category() const {
  return "DataHandling";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractMonitorWorkspace::summary() const {
  return "Retrieves a workspace of monitor data held within the input "
         "workspace, if present.";
}

/** Initialize the algorithm's properties.
 */
void ExtractMonitorWorkspace::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "A data workspace that holds a monitor workspace within.");
  declareProperty(
      new WorkspaceProperty<>("MonitorWorkspace", "", Direction::Output),
      "The workspace containing only monitor data relating to the main data in "
      "the InputWorkspace.");
  declareProperty(
      "ClearFromInputWorkspace", true,
      "Whether to hold onto the monitor workspace within "
      "the input workspace. The default is not to, but if you are running this "
      "algorithm in the post-processing "
      "step of a live data run then you will need this to be false.");
}

/** Execute the algorithm.
 */
void ExtractMonitorWorkspace::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto monitorWS = inputWS->monitorWorkspace();

  if (!monitorWS) {
    throw std::invalid_argument(
        "The input workspace does not hold a monitor workspace");
  }

  setProperty("MonitorWorkspace", monitorWS);
  // Now clear off the pointer on the input workspace, if desired
  const bool clearPointer = getProperty("ClearFromInputWorkspace");
  if (clearPointer)
    inputWS->setMonitorWorkspace(MatrixWorkspace_sptr());
}

} // namespace DataHandling
} // namespace Mantid
