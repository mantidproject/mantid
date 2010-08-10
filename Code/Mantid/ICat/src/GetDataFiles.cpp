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
			declareProperty<long long>("InvestigationId",-1,mustBePositive,"Id of the selected investigation");
		
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the file data search details");
			declareProperty("DataFiles",false,"Use this boolean option to filter log files.\n"
				"The default option is set to false and loads all the files assocaited to the selected investigation.");
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
			
			long long investigationId = getProperty("InvestigationId");
			bool bDataFiles =getProperty("DataFiles");
			
			//API::ITableWorkspace_sptr outputws ;
			API::ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable("TableWorkspace");
			CSearchHelper searchobj;	
			int ret_advsearch=searchobj.getDataFiles(investigationId,bDataFiles,ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,outputws);
			return outputws;
		}
		
		
	}
}