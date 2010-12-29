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
DECLARE_ALGORITHM( RenameWorkspace)

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void RenameWorkspace::init()
{
  declareProperty(new WorkspaceProperty<Workspace> ("InputWorkspace", "", Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace> ("OutputWorkspace", "", Direction::Output));
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

  WorkspaceGroup_sptr ingrp_sptr = boost::dynamic_pointer_cast<WorkspaceGroup>(localworkspace);
  // get the workspace name
  std::string inputwsName = localworkspace->getName();
  // get the output workspace name
  std::string outputwsName = getPropertyValue("OutputWorkspace");
  AnalysisDataServiceImpl& data_store = AnalysisDataService::Instance();
  if (!ingrp_sptr)
  {
    // Assign it to the output workspace property
    setProperty("OutputWorkspace", localworkspace);
  }
  else
  {
    //create output group ws 
    WorkspaceGroup_sptr outgrp_sptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outgrp_sptr));
   
    //counter used to name the group members
    int count = 0;
    std::string outputWorkspace = "OutputWorkspace";
    // Get members of the inpot group workspace
    const std::vector<std::string> names = ingrp_sptr->getNames();
    std::vector<std::string>::const_iterator citr = names.begin();
    //loop thorugh input ws group members
    for (; citr != names.end(); ++citr)
    {
      try
      {
        // new name of the member workspaces
        std::stringstream suffix;
        suffix << ++count;
        std::string outws = outputWorkspace + "_" + suffix.str();
        std::string wsName = outputwsName + "_" + suffix.str();

        //retrieving the workspace pointer
        Workspace_sptr ws_sptr = data_store.retrieve((*citr));
        declareProperty(new WorkspaceProperty<Workspace> (outws, wsName,
            Direction::Output));
        setProperty(outws, ws_sptr);

        // adding the workspace members to group
        outgrp_sptr->add(wsName);

        //remove the input group workspace  members from ADS

        AnalysisDataService::Instance().remove((*citr));
      } catch (Kernel::Exception::NotFoundError&ex)
      {
        g_log.error() << ex.what() << std::endl;
      }

    }

  }
  // Post notice that a workspace has been renamed
  data_store.notificationCenter.postNotification(new WorkspaceRenameNotification(inputwsName,
      outputwsName));
  //remove the input workspace from the analysis data service
  data_store.remove(getPropertyValue("InputWorkspace"));

}

} // namespace Algorithms
} // namespace Mantid
