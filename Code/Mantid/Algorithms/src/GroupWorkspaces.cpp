//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GroupWorkspaces.h"

namespace Mantid
{
	namespace Algorithms
	{
		DECLARE_ALGORITHM(GroupWorkspaces)
		using namespace Kernel;
		using namespace API;
		///Initialisation method
		void GroupWorkspaces::init()
		{
			declareProperty( new ArrayProperty<std::string>("InputWorkspaces"),
				"Name of the Input Workspaces to Group" );
			declareProperty( new WorkspaceProperty<WorkspaceGroup>("OutputWorkspace","",Direction::Output),
				"Name of the workspace to be created as the output of grouping ");
		}
		/** Executes the algorithm
		*  @throw std::exception If theselected workspaces are not of same types
		*/
		void GroupWorkspaces::exec()
		{
			try
			{
				std::vector<std::string> inputWS=getProperty("InputWorkspaces");
				std::string newGroup=getPropertyValue("OutputWorkspace");
				if(inputWS.size()<2)
				{	throw std::exception("Select atleast two workspaces to group ");
				}
				//creates workspace group
				WorkspaceGroup_sptr outputGrpWS=WorkspaceGroup_sptr(new WorkspaceGroup);
				if(outputGrpWS)
				{
					//add "NewGroup" to  workspace group
					outputGrpWS->add(newGroup);	
					setProperty("OutputWorkspace",outputGrpWS);
					// iterate through the selected input workspaces
					std::vector<std::string>::const_iterator itr;
					for (itr=inputWS.begin();itr!=inputWS.end();itr++)
					{	
						std::vector<std::string> groupVec=outputGrpWS->getNames();
						if(groupVec.size()>1)
						{
							std::string firstWS=groupVec[1];
							if(isCompatibleWorkspaces(firstWS,(*itr)))
								outputGrpWS->add((*itr));
							else
							{  
								throw std::exception("Selected workspaces are not of same Types.\n"
									"Check the selected workspaces and ensure that they are of same types to group");
							}
						}
						else
							outputGrpWS->add((*itr));
					}//end of for loop for selected workspace iteration
				}
			}
			catch(std::invalid_argument &)
			{
				//g_log.error()<<"Error:"<<ex.what()<<std::endl;
				throw;
			}
			catch(std::runtime_error& )
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
		/** checks the input workspaces are of same types
		*  @param firstWS    first workspace added to group vector
		*  @param newWStoAdd   new workspace to add to vector
		*  @retval boolean  true if two workspaces are of same types else false
		*/
		bool GroupWorkspaces::isCompatibleWorkspaces(const std::string & firstWS,const std::string& newWStoAdd )
		{
			std::string firstWSTypeId("");
			bool bStatus=0;
			Workspace_sptr wsSptr1=Mantid::API::AnalysisDataService::Instance().retrieve(firstWS);
			firstWSTypeId=wsSptr1->id();

			//check the typeid  of the  next workspace 
			std::string wsTypeId("");
			Workspace_sptr wsSptr=Mantid::API::AnalysisDataService::Instance().retrieve(newWStoAdd);
			if(wsSptr) wsTypeId=wsSptr->id();
			(firstWSTypeId==wsTypeId) ? (bStatus=true ) :( bStatus= false);
			//}
			return bStatus;
		}

	}
}