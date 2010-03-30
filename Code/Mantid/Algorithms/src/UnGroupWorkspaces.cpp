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
			try
			{				
				std::string inputws=getProperty("InputWorkspace");
				// send this notification to mantidplot to add the group member workspaces to mantid tree
				// this is bcoz the ADS remove will delete the group workspace and its member workspaces from mantid tree
				//send this before ADS remove to mantidplot
				Mantid::API::AnalysisDataService::Instance().notificationCenter.postNotification(
					new Kernel::DataService<Workspace>::UnGroupWorkspaceNotification(inputws));	

				
				Workspace_sptr wsSptr=API::AnalysisDataService::Instance().retrieve(inputws);
				if(wsSptr)
				{
					//if this is group workspace
					WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
					if(wsGrpSptr)
					{	//delete the group workspace
						API::AnalysisDataService::Instance().remove(inputws);

					}
					else
					{
						throw std::runtime_error("Selected Workspace is not a Groupworkspace to Ungroup ");
					}
				}

			}
			catch(std::invalid_argument &)
			{
				throw;
			}
			catch(Mantid::Kernel::Exception::NotFoundError& )
			{//if not a valid object in analysis data service
				throw ;
			}
			catch(std::runtime_error&)
			{
				throw;
			}
			catch(std::exception& )
			{
				throw;
			}

		}

	}
}
