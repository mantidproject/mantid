/*WIKI* 
This algorithm takes two or more workspaces as input and creates an output workspace group.
 *WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidKernel/MandatoryValidator.h"

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
      this->setWikiSummary("Takes workspaces as input and group similar workspaces together.");
      declareProperty(new ArrayProperty<std::string> ("InputWorkspaces", boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
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
      //creates workspace group pointer
      WorkspaceGroup_sptr outgrp_sptr = WorkspaceGroup_sptr(new WorkspaceGroup);

      setProperty("OutputWorkspace", outgrp_sptr);

      std::string outputWorkspace = "OutputWorkspace";
      int count = 0;
      std::vector<std::string>::const_iterator citr;
      std::string firstWs("");
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
            addworkspacetoGroup(outgrp_sptr, ws_sptr, firstWs);
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
          addworkspacetoGroup(outgrp_sptr, ws_sptr, firstWs);
        }
      }//end of for loop for input workspaces

      // Notify listeners that a new grop has been created
      Mantid::API::AnalysisDataService::Instance().notificationCenter.postNotification(
          new WorkspacesGroupedNotification(inputworkspaces));

    }

    ///** checks the input workspaces are of same types
    // *  @param wsName ::    name of the workspace to be added
    // *  @param firstWs ::   the first workspace type (not including table workspaces)
    // *  @retval boolean  true if two workspaces are of same types else false
    // */
    bool GroupWorkspaces::isCompatibleWorkspaces(Workspace_sptr ws, std::string& firstWs)
    {
      bool bStatus(true);
      //check to see if compatible with each other (exception for TableWorkspaces.)
      if ( ws->id() != "TableWorkspace" )
      {
        if (firstWs == "")
        {
          firstWs = ws->id();
        }
        else
        {
          if (ws->id() != firstWs)
          {
            bStatus = false;
          }
        }
      }
      return bStatus;
    }

    ///** add workspace to groupworkspace
    // *  @param outgrp_sptr ::    shared pointer to groupworkspace
    // *  @param wsName ::   name of the workspace to add to group
    // *  @param firstWs ::   the first workspace type (not including table workspaces)
    // */
    void GroupWorkspaces::addworkspacetoGroup(WorkspaceGroup_sptr outgrp_sptr, Workspace_sptr ws, std::string &firstWs)
    {
      if (!outgrp_sptr->isEmpty())
      {
        if( isCompatibleWorkspaces( ws, firstWs ) )
        {
          outgrp_sptr->addWorkspace(ws);
        }
        else
        {
          throw std::runtime_error("Selected workspaces are not of same Types.\n"
              "Check the selected workspaces and ensure that they are of same types to group");
        }
      }
      else
      {
        outgrp_sptr->addWorkspace(ws);
      }
    }

  }
}
