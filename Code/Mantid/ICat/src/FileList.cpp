#include "MantidICat/FileList.h"
#include"MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include"MantidICat/SearchHelper.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;
		/// Init method
		void CFileList::init()
		{
			BoundedValidator<double>* mustBePositive = new BoundedValidator<double>();
			mustBePositive->setLower(0.0);

			declareProperty("StartRun",0.0,mustBePositive,"The start run number");
			declareProperty("EndRun",0.0,mustBePositive->clone(),"The end run number");
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the icat search result");
		}
		/// Exec Method
		void CFileList::exec()
		{
		   API::ITableWorkspace_sptr ws_sptr=doFileSearch();
			setProperty("OutputWorkspace",ws_sptr);
		}
		/// This method does teh file search
		API::ITableWorkspace_sptr CFileList::doFileSearch()
		{
			/*ns1__searchByAdvanced request;
			setRequestParameters(request);
			
			API::ITableWorkspace_sptr outputws;
			ns1__searchByAdvancedResponse response;*/
//
//			double dstartRun=getProperty("StartRun");
//			double dendRun=getProperty("EndRun");
//
			API::ITableWorkspace_sptr outputws;
//			CSearchHelper searchobj;
			//int ret_advsearch=searchobj.doSearchByRunNumber(dstartRun,dendRun,ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,outputws);
			//int ret_advsearch=searchobj.doSearchByRunNumber(instrList,dstartRun,dendRun,outputws);
			//if(ret_advsearch!=0)
			//{
			  //need to throw proper exception
				//throw ;
			//}
           	return outputws;
		}
		
	}
}
