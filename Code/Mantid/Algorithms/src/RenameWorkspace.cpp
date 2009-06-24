//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RenameWorkspace.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RenameWorkspace)

using namespace Kernel;
using namespace API;

// Get a reference to the logger
Logger& RenameWorkspace::g_log = Logger::get("RenameWorkspace");

/** Initialisation method.
 *
 */
void RenameWorkspace::init()
{
  declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void RenameWorkspace::exec()
{
	if (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace"))
	{
		throw std::invalid_argument("The input and output workspace names must be different");
	}
  // Get the input workspace
  Workspace_sptr localworkspace = getProperty("InputWorkspace");

  // Assign it to the output workspace property
  setProperty("OutputWorkspace",localworkspace);

	//remove the input workspace from the analysis data service
	AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace"));

  return;
}

} // namespace Algorithms
} // namespace Mantid
