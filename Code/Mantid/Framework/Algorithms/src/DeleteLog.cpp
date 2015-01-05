#include "MantidAlgorithms/DeleteLog.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {
using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DeleteLog)

//----------------------------------------------------------------------------------------------
/// @copydoc Algorithm::name
const std::string DeleteLog::name() const { return "DeleteLog"; }

/// @copydoc Algorithm::version
int DeleteLog::version() const { return 1; }

/// @copydoc Algorithm::category
const std::string DeleteLog::category() const { return "DataHandling\\Logs"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DeleteLog::init() {
  declareProperty(new WorkspaceProperty<>("Workspace", "", Direction::InOut),
                  "In/out workspace containing the logs. The workspace is "
                  "modified in place");
  declareProperty("Name", "",
                  boost::make_shared<MandatoryValidator<std::string>>(), "",
                  Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DeleteLog::exec() {
  MatrixWorkspace_sptr logWS = getProperty("Workspace");
  std::string logName = getProperty("Name");
  auto &run = logWS->mutableRun();
  if (run.hasProperty(logName)) {
    run.removeLogData(logName);
  } else {
    g_log.warning() << "Unable to delete log '" << logName
                    << "' from the given workspace as it does not exist.\n";
  }
}

} // namespace Algorithms
} // namespace Mantid
