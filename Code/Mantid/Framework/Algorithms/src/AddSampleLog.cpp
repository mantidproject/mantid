//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidKernel/Exception.h"
#include <string>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddSampleLog)

using namespace Kernel;
using namespace API;

void AddSampleLog::init()
{
  this->setWikiSummary("Used to insert a single string into the sample in a workspace");
  this->setOptionalMessage("Used to insert a single string into the sample in a workspace");

  declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut),
    "Workspace to add the log entry to");
  declareProperty("LogName", "", new MandatoryValidator<std::string>,
    "The name that will identify the log entry");
  declareProperty("LogText", "",
    "The content of the log");
}

void AddSampleLog::exec()
{
  // A pointer to the workspace to add a log to
  MatrixWorkspace_sptr wSpace = getProperty("Workspace");
  // we're going to edit the workspaces run details so get a non-const reference to it
  Run &theRun = wSpace->mutableRun();

  // get the data that the user wants to add
  std::string propName = getProperty("LogName");
  std::string propValue = getProperty("LogText");

  theRun.addLogData(new PropertyWithValue<std::string>(propName, propValue));

  setProperty("Workspace", wSpace);
}

} // namespace Algorithms
} // namespace Mantid
