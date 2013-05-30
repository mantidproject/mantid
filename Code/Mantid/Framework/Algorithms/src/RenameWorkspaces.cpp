/*WIKI* 

Renames a list of workspaces to a different name in the data service.
This renaming is done by appending a suffix or adding a prefix to the old names.
The Renaming is implemented by calling RenameWorspace as a child algorithm having defined the output workspace appropriately.

If run on a group workspace, the members of the group will be renamed if their names follow the pattern groupName_1, groupName_2, etc. (they will be renamed to newName_1, newname_2, etc.). 
Otherwise, only the group itself will be renamed - the members will keep their previous names.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RenameWorkspaces.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM( RenameWorkspaces)

void RenameWorkspaces::initDocs()
{
  this->setWikiSummary("Used to Rename a list workspaces in the [[Analysis Data Service]]. This is the algorithm that is run if 'Rename' is chosen from the context menu of a workspace and several workspaces have been selected.");
  this->setOptionalMessage("Rename the Workspace.");
}

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void RenameWorkspaces::init()
{
  declareProperty(new WorkspaceProperty<Workspace> ("InputWorkspace", "", Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace> ("OutputWorkspace", "", Direction::Output));
  declareProperty("Prefix", false,
      "If true, then Output Workspace if prefixed, else it is suffixed.",
      Direction::Input);

}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void RenameWorkspaces::exec()
{
  // Get the input workspace list
  std::string inputwsName = getPropertyValue("InputWorkspace");
  // get the output workspace affix
  std::string outputwsName = getPropertyValue("OutputWorkspace");

  if (getPropertyValue("OutputWorkspace") == "")
  {
    throw std::invalid_argument("The no prefix or suffix has been supplied");
  }

  // Convert the comma separated input workspace list to an array

  // loop over array and rename each workspace


}

} // namespace Algorithms
} // namespace Mantid

