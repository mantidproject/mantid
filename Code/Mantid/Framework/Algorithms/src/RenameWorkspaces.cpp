/*WIKI* 

Renames a list of workspaces to a different name in the data service.
This renaming is done by either replacing with new names in a list or adding a prefix, suffix or both prefix and suffix.
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
  std::vector<std::string> inputWsName = getProperty("InputWorkspaces");

  // Get the workspace name list
  std::vector<std::string> newWsName = getProperty("WorkspaceNames");
  // Get the prefix and suffix
  std::string prefix = getPropertyValue("Prefix");
  std::string suffix = getPropertyValue("Suffix");

  // Check properties
  if( newWsName.size() == 0 && prefix == "" && suffix == "")
  {
      throw std::invalid_argument("No list of Workspace names, prefix or suffix has been supplied.");
  }
  if( newWsName.size() > 0 && ( prefix != "" || suffix != ""))
  {
      throw std::invalid_argument("Both a list of workspace names and a prefix or suffix has been supplied.");
  }
  if( newWsName.size() > 1) 
  {
     for( size_t i=0; i<newWsName.size()-1; ++i) 
     {
       for( size_t j=i+1; j<newWsName.size(); ++j)
       {
         if(newWsName[i] == newWsName[j])
         {
           throw std::invalid_argument("Duplicate '"+newWsName[i]+"' found in WorkspaceNames.");
         }
       }
     }
  }


  size_t nWs = inputWsName.size();
  if( newWsName.size() > 0) {
    // We are using a list of new names
    if (nWs > newWsName.size() )
    { 
      nWs = newWsName.size();
      // could issue warning here
    }
  } 
  else
  { // We are using prefix and/or suffix
    // Build new names.
    for (size_t i=0; i<nWs; ++i)
    {
      newWsName.push_back(prefix+inputWsName[i]+suffix);
    }
  }

  // loop over array and rename each workspace
  for (size_t i=0; i<nWs; ++i)
  {
    std::ostringstream os;
    os << "OutputWorkspace_" << i+1;
    declareProperty(new WorkspaceProperty<Workspace>(os.str(), newWsName[i], Direction::Output));
    auto alg = createChildAlgorithm("RenameWorkspace");
    alg->setPropertyValue("InputWorkspace", inputWsName[i]);
    alg->setPropertyValue("OutputWorkspace", newWsName[i]);

    alg->executeAsChildAlg();

    Workspace_sptr renamedWs = alg->getProperty("OutputWorkspace"); 
    setProperty(os.str(), renamedWs);
  }

}

} // namespace Algorithms
} // namespace Mantid

