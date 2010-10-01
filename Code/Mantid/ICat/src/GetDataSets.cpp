#include "MantidICat/GetDataSets.h"
#include "MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidICat/SearchHelper.h"
#include "MantidICat/ErrorHandling.h" 
#include "MantidICat/Session.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;
		DECLARE_ALGORITHM(CGetDataSets)
		/// Initialisation methods
		void CGetDataSets::init()
		{
			BoundedValidator<long long>* mustBePositive = new BoundedValidator<long long>();
			mustBePositive->setLower(0);
			declareProperty<long long>("InvestigationId",-1,mustBePositive,"Id of the selected investigation");

			//declareProperty("Title","","The title of the investigation to do data sets search ");
			//declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("InputWorkspace","",Direction::Input),
			//	"The name of the workspace which stored the last icat investigations search result");

			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the result of datasets search ");
		}
		/// exec methods
		void CGetDataSets::exec()
		{
			if(Session::Instance().getSessionId().empty())
			{
				throw std::runtime_error("Please login to ICat using the ICat:Login menu provided to access ICat data.");
			}
		   API::ITableWorkspace_sptr ws_sptr = doDataSetsSearch();
		   setProperty("OutputWorkspace",ws_sptr);
		}
		/// This method gets the data sets for a given investigation id
		API::ITableWorkspace_sptr CGetDataSets::doDataSetsSearch()
		{
			long long investigationId = getProperty("InvestigationId");
			
			API::ITableWorkspace_sptr outputws =WorkspaceFactory::Instance().createTable("TableWorkspace");

			CSearchHelper searchobj;
			//search datasets for a given investigation id using ICat api.
			int ret_advsearch = searchobj.doDataSetsSearch(investigationId,
				ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATASET_USCOREPARAMETERS_USCOREONLY,outputws);
			(void) ret_advsearch;
           	return outputws;
		}
		
		
	}
}
