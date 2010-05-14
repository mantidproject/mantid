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
      std::string inputws = getProperty("InputWorkspace");
      Mantid::API::AnalysisDataServiceImpl & data_store = Mantid::API::AnalysisDataService::Instance();
      Workspace_sptr wsSptr = data_store.retrieve(inputws);
      WorkspaceGroup_sptr wsGrpSptr = boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
      if( !wsGrpSptr )
      {
	throw std::runtime_error("Selected Workspace is not a WorkspaceGroup");
      }

      // Notify observers that a WorkspaceGroup is about to be unrolled
      data_store.notificationCenter.postNotification(new Mantid::API::WorkspaceUnGroupingNotification(inputws, wsSptr));
      if(wsSptr)
      {
	//if this is group workspace

	if(wsGrpSptr)
	{
	  API::AnalysisDataService::Instance().remove(inputws);
	}
	else
	{
	
	}
      }
    }

  }
}
