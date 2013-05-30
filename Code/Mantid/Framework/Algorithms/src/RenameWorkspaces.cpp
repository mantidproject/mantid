/*WIKI* 

Renames a list of workspaces to a different name in the data service.
This renaming is done by either replacing with new names in a list or adding a prefix, suffix or both.
The Renaming is implemented by calling RenameWorkspace as a child algorithm having defined the output workspace appropriately.

If run on a group workspace, the members of the group will be renamed in the same manner as done by RemameWorkspace

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RenameWorkspaces.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
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
  declareProperty(new ArrayProperty<std::string> ("InputWorkspaces", boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "Names of the Input Workspaces");
  // WorkspaceNames - List of new names
  declareProperty(new ArrayProperty<std::string>("WorkspaceNames",Direction::Input), "New Names of the Workspaces");
  // --or--
  // Prefix
  declareProperty("Prefix",std::string(""),"Prefix to add to input workspace names",Direction::Input);
  // Suffix 
  declareProperty("Suffix",std::string(""),"Suffix to add to input workspace names",Direction::Input);
  
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void RenameWorkspaces::exec()
{
  // Get the input workspace list
  std::vector<std::string> inputwsName = getProperty("InputWorkspaces");

  //// get the output workspace affix
  //std::string outputwsName = getPropertyValue("OutputWorkspace");
  //// get the prefix indicator
  //bool isPrefix = getProperty("isPrefix");

  //if (getPropertyValue("OutputWorkspace") == "")
  //{
  //  throw std::invalid_argument("The no prefix or suffix has been supplied");
  //}

  // Convert the comma separated input workspace list to an array

  // loop over array and rename each workspace

  //declareProperty("OutputWorkspace_" + 1);


}

} // namespace Algorithms
} // namespace Mantid

