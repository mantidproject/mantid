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
			//declareProperty( new WorkspaceProperty<WorkspaceGroup>("InputWorkspace","",Direction::Input),
			//	"Name of the Input Workspace to UnGroup" );
			declareProperty( new ArrayProperty<std::string>("InputWorkspaces"),
				"Name of the Input Workspaces to UnGroup" );
		//	declareProperty( new WorkspaceProperty<WorkspaceGroup>("OutputWorkspace","",Direction::Output),
		//		"Name of the workspace to be created as the output of grouping ");
		}
		/** Executes the algorithm
		*  @throw std::exception If the selected workspace ais not group workspace
		*/
		void UnGroupWorkspace::exec()
		{
			try
			{
				//std::string inputWSName=getProperty("InputWorkspace");
				std::vector<std::string> inputWS=getProperty("InputWorkspaces");
				//g_log.error()<<"InputWorkspace: "<< inputWSName<<std::endl;
				std::vector<std::string>::const_iterator itr;
				for (itr=inputWS.begin();itr!=inputWS.end();itr++)
				{
					//checks it's group
					Workspace_sptr wsSptr=API::AnalysisDataService::Instance().retrieve((*itr));
					if(wsSptr)
					{
						WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
						if(wsGrpSptr)
						{//if it's a group delete the group workspace
							API::AnalysisDataService::Instance().remove((*itr));
							//g_log.error()<<"InputWorkspace: "<< inputWSName<<" Removed from ADS"<<std::endl;
						}
						else
							throw std::exception("Selected Workspace is not a Group to Ungroup ");
					}
				}
			
			}
			catch(std::invalid_argument &)
			{
				//g_log.error()<<"Error:"<<ex.what()<<std::endl; 
				throw;
			}
			catch(Mantid::Kernel::Exception::NotFoundError&)
			{//if not a valid object in analysis data service
				//g_log.error()<<"Error: "<< e.what()<<std::endl;
				throw;
			}
			catch(std::runtime_error&)
			{
				//g_log.error()<<"Error:"<<ex.what()<<std::endl; 
				throw;
			}
			catch(std::exception& )
			{
				//g_log.error()<<"Error:"<<ex.what()<<std::endl;
				throw;
			}

		}

	}
}