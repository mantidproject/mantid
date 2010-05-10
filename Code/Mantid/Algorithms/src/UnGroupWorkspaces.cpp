//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnGroupWorkspaces.h"

namespace Mantid
{
  namespace Algorithms
  {
    
    DECLARE_ALGORITHM(UnGroupWorkspace)
    
    using namespace Kernel;
    using namespace API;
    
    ///Initialisation method
    void UnGroupWorkspace::init()
    {
      declareProperty("InputWorkspace","","Name of the input workspace to ungroup" );

    }
    
    /** Executes the algorithm
     *  @throw std::runtime_error If the selected workspace is not a group workspace
     */
    void UnGroupWorkspace::exec()
    {
      std::string inputws=getProperty("InputWorkspace");
      // send this notification to mantidplot to add the group member workspaces to mantid tree
      // this is bcoz the ADS remove will delete the group workspace and its member workspaces from mantid tree
      //send this before ADS remove to mantidplot
      Mantid::API::AnalysisDataService::Instance().notificationCenter.postNotification(new Mantid::API::WorkspaceUnGroupedNotification(inputws));
      
      Workspace_sptr wsSptr=API::AnalysisDataService::Instance().retrieve(inputws);
      if(wsSptr)
      {
	//if this is group workspace
	WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
	if(wsGrpSptr)
	{
	  API::AnalysisDataService::Instance().remove(inputws);
	}
	else
	{
	  throw std::runtime_error("Selected Workspace is not a Groupworkspace to Ungroup ");
	}
      }
    }

  }
}
