#include "MantidICat/GetDataFiles.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidICat/SearchHelper.h"
#include "MantidICat/ErrorHandling.h" 

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CGetDataFiles)
		/// Initialising the algorithm
		void CGetDataFiles::init()
		{
			BoundedValidator<long long>* mustBePositive = new BoundedValidator<long long>();
			mustBePositive->setLower(0);
			declareProperty<long long>("InvestigationId",0,mustBePositive,"Id of the selected investigation");

			//declareProperty("Title","","The title of the investigation to do data search ");

			//declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("InputWorkspace","",Direction::Input),
			//	"The name of the workspace which stored the last icat investigation search result");

			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the file data search details");
		}
		//execute the algorithm
		void CGetDataFiles::exec()
		{
		   API::ITableWorkspace_sptr ws_sptr=doDataFilesSearch();
			setProperty("OutputWorkspace",ws_sptr);
		}
		/// This method returns a set of data files for a given investigationid
		API::ITableWorkspace_sptr CGetDataFiles::doDataFilesSearch()
		{
			//std::string invstTitle=getProperty("Title");
			//input workspace
			//API::ITableWorkspace_sptr inputws_sptr=getProperty("InputWorkspace");

			//int row=0;
			//const int col=2; //need to find a way to get column index in table workspace.
			//long long investigationId=0;
			//try
			//{
			//
			//inputws_sptr->find(invstTitle,row,col);
			//investigationId=inputws_sptr->cell<long long >(row,col-2);
			//}
			//catch(std::range_error&)
			//{
			//	throw;
			//}
			//catch(std::out_of_range&)
			//{
			//	throw;
			//}
			//catch(std::runtime_error&)
			//{
			//	throw;
			//}

			long long investigationId = getProperty("InvestigationId");
			
			API::ITableWorkspace_sptr outputws;
			CSearchHelper searchobj;	
			int ret_advsearch=searchobj.getDataFiles(investigationId,ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,outputws);
			return outputws;
		}
		
		
	}
}