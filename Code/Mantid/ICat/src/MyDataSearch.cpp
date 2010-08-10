#include "MantidICat/MyDataSearch.h"
#include "MantidICat/SearchHelper.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CMyDataSearch)
		/// Initialisation method.
		void CMyDataSearch::init()
		{
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the result of MyData search ");
		}
		/// Execution method.
		void CMyDataSearch::exec()
		{			
			API::ITableWorkspace_sptr ws_sptr=doMyDataSearch();
			setProperty("OutputWorkspace",ws_sptr);
		}
		/* This method does looged in users investigations search.
		 * @returns shared pointer to table workspace which stores the data
		 */
		API::ITableWorkspace_sptr  CMyDataSearch::doMyDataSearch()
		{
			API::ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable("TableWorkspace"); 
			CSearchHelper searchobj;
			searchobj.doMyDataSearch(outputws);
			return outputws;
		}
	}
}