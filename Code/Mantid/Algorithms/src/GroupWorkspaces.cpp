//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GroupWorkspaces.h"

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM( GroupWorkspaces)

using namespace Kernel;
using namespace API;

///Initialisation method
void GroupWorkspaces::init()
{
  declareProperty(new ArrayProperty<std::string> ("InputWorkspaces"),
      "Name of the Input Workspaces to Group");
  declareProperty(new WorkspaceProperty<WorkspaceGroup> ("OutputWorkspace", "", Direction::Output),
      "Name of the workspace to be created as the output of grouping ");
}

/** Executes the algorithm
 *  @throw std::runtime_error If theselected workspaces are not of same types
 */
void GroupWorkspaces::exec()
{
  const std::vector<std::string> inputworkspaces = getProperty("InputWorkspaces");
  if (inputworkspaces.size() < 2)
  {
    throw std::runtime_error("Select atleast two workspaces to group ");
  }
  // output groupworkspace name
  std::string newGroup = getPropertyValue("OutputWorkspace");
  //creates workspace group pointer
  WorkspaceGroup_sptr outgrp_sptr = WorkspaceGroup_sptr(new WorkspaceGroup);

  setProperty("OutputWorkspace", outgrp_sptr);

  std::string outputWorkspace = "OutputWorkspace";
  int count = 0;
  std::vector<std::string>::const_iterator citr;
  // iterate through the input workspaces
  for (citr = inputworkspaces.begin(); citr != inputworkspaces.end(); ++citr)
  {
    //if the input workspace is a group disassemble the group and add to output group
    Workspace_sptr inws_sptr = AnalysisDataService::Instance().retrieve(*citr);
    WorkspaceGroup_sptr ingrp_sptr = boost::dynamic_pointer_cast<WorkspaceGroup>(inws_sptr);
    if (ingrp_sptr)
    {
      std::vector<std::string> names = ingrp_sptr->getNames();
      std::vector<std::string>::const_iterator itr = names.begin();
      for (; itr != names.end(); ++itr)
      {
        std::stringstream suffix;
        suffix << ++count;
        std::string outws = outputWorkspace + "_" + suffix.str();
        //retrieving the workspace pointer
        Workspace_sptr ws_sptr = AnalysisDataService::Instance().retrieve((*itr));
        //workspace name
        std::string wsName = (*itr);
        //declaring the member output workspaces property
        declareProperty(new WorkspaceProperty<Workspace> (outws, wsName,
            Direction::Output));
        setProperty(outws, ws_sptr);

        //add to output group
        addworkspacetoGroup(outgrp_sptr, (*itr));
      }
      inws_sptr.reset();
      ingrp_sptr.reset();
      AnalysisDataService::Instance().remove(*citr);
    }
    else
    {
      std::stringstream suffix;
      suffix << ++count;
      std::string outws = outputWorkspace + "_" + suffix.str();
      //retrieving the workspace pointer
      Workspace_sptr ws_sptr = AnalysisDataService::Instance().retrieve((*citr));
      //workspace name
      std::string wsName = (*citr);
      //declaring the member output workspaces property
      declareProperty(new WorkspaceProperty<Workspace> (outws, wsName, Direction::Output));
      setProperty(outws, ws_sptr);
      //add to output group
      addworkspacetoGroup(outgrp_sptr, (*citr));
    }

  }//end of for loop for input workspaces

  // Notify listeners that a new grop has been created
  Mantid::API::AnalysisDataService::Instance().notificationCenter.postNotification(
      new WorkspacesGroupedNotification(inputworkspaces));

}

/** checks the input workspaces are of same types
 *  @param firstWS    first workspace added to group vector
 *  @param newWStoAdd   new workspace to add to group
 *  @retval boolean  true if two workspaces are of same types else false
 */
bool GroupWorkspaces::isCompatibleWorkspaces(const std::string & firstWS, const std::string& newWStoAdd)
{
  bool bStatus = 0;
  try
  {
    std::string firstWSTypeId;

    Workspace_sptr wsSptr1 = Mantid::API::AnalysisDataService::Instance().retrieve(firstWS);
    firstWSTypeId = wsSptr1->id();

    //check the typeid  of the  next workspace
    std::string wsTypeId("");
    Workspace_sptr wsSptr = Mantid::API::AnalysisDataService::Instance().retrieve(newWStoAdd);
    if (wsSptr)
    {
      wsTypeId = wsSptr->id();
    }
    (firstWSTypeId == wsTypeId) ? (bStatus = true) : (bStatus = false);
  } catch (Kernel::Exception::NotFoundError& e)
  {
    g_log.error() << e.what() << std::endl;
    bStatus = false;
  }

  return bStatus;
}

/** add workspace to groupworkspace
 *  @param outgrp_sptr    shared pointer to groupworkspace
 *  @param wsName   name of the workspace to add to group
 */
void GroupWorkspaces::addworkspacetoGroup(WorkspaceGroup_sptr outgrp_sptr, const std::string &wsName)
{
  std::vector<std::string> groupVec = outgrp_sptr->getNames();
  if (groupVec.size() > 1)
  {
    std::string firstws = groupVec[0];
    if (isCompatibleWorkspaces(firstws, wsName))
    {
      outgrp_sptr->add(wsName);
    }
    else
    {
      throw std::runtime_error("Selected workspaces are not of same Types.\n"
        "Check the selected workspaces and ensure that they are of same types to group");
    }
  }
  else
  {
    outgrp_sptr->add(wsName);
	
  }
}

}
}
