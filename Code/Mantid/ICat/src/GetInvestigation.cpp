#include "MantidICat/GetInvestigation.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidICat/SearchHelper.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;
		void CGetInvestigation::init()
		{
			declareProperty("Title","","The title of the investigation to do data search ");
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("InputWorkspace","",Direction::Input),
				"The name of the workspace which stored the last icat investigation search result");
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the file data search details");
		}
		void CGetInvestigation::exec()
		{
		   API::ITableWorkspace_sptr ws_sptr=doInvestigationSearch();
			setProperty("OutputWorkspace",ws_sptr);
		}
		API::ITableWorkspace_sptr CGetInvestigation::doInvestigationSearch()
		{
			std::string invstTitle=getProperty("Title");
			//input workspace
			API::ITableWorkspace_sptr inputws_sptr=getProperty("InputWorkspace");

			int row=0;
			const int col=2; //need to find a way to get column index in table workspace.
			long long investigationId=0;
			try
			{
			inputws_sptr->find(invstTitle,row,col);
			investigationId=inputws_sptr->cell<long long >(row,col-2);
			}
			catch(std::range_error&)
			{
				throw;
			}
			catch(std::runtime_error&)
			{
				throw;
			}
			
			API::ITableWorkspace_sptr outputws;
			CSearchHelper searchobj;	
			int ret_advsearch=searchobj.dogetInvestigationIncludes(investigationId,ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,outputws);
			//int ret_advsearch=searchobj.doSearchByRunNumber(instrList,dstartRun,dendRun,outputws);
			if(ret_advsearch!=0)
			{
			  //need to throw proper exception
				throw ;
			}
           	return outputws;
		}
		
		
	}
}