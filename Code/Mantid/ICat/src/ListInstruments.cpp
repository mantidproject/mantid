#include "MantidICat/ListInstruments.h"
#include "MantidICat/SearchHelper.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidICat/Session.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CListInstruments)
		/// Init method
		void CListInstruments::init()
		{
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
				"The name of the table workspace that will be created to store the instruments list");
		}
		/// exec method
		void CListInstruments::exec()
		{
			//API::ITableWorkspace_sptr ws_sptr=
			if(Session::Instance().getSessionId().empty())
			{
				throw std::runtime_error("Please login to ICat using the ICat:Login menu provided to access ICat data.");
			}
			ITableWorkspace_sptr ws_sptr= WorkspaceFactory::Instance().createTable("TableWorkspace");
			listInstruments(ws_sptr); 
			setProperty("OutputWorkspace",ws_sptr);
		}

		/**This method returns list of instruments from ICat DB
		 *@param ws_sptr - shared pointer to table workspace
		 */
		void CListInstruments::listInstruments(ITableWorkspace_sptr& ws_sptr)
		{
			CSearchHelper searchObj;
			searchObj.listInstruments(ws_sptr);
		}

	}
}

