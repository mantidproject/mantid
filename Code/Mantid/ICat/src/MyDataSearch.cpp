#include "MantidICat/MyDataSearch.h"
#include "MantidICat/SearchHelper.h"
#include "MantidICat/Session.h"

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
			if(Session::Instance().getSessionId().empty())
			{
				throw std::runtime_error("Please login to ICat using the ICat:Login menu provided to access ICat data.");
			}
			API::ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable("TableWorkspace");
			doMyDataSearch(outputws);
			setProperty("OutputWorkspace",outputws);
		
		}
		/* This method does logged in users investigations search.
		 * @param outputws pointer to table workspace which stores the data
		 */
		void  CMyDataSearch::doMyDataSearch(API::ITableWorkspace_sptr& outputws)
		{			 
			CSearchHelper searchobj;
			searchobj.doMyDataSearch(outputws);
			
		}
	}
}
