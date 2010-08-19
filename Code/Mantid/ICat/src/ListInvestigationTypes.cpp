#include "MantidICat/ListInvestigationTypes.h"
#include "MantidICat/SearchHelper.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/TableWorkspace.h"


namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CListInvestigationTypes)
		/// Init method
		void CListInvestigationTypes::init()
		{
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
				"The name of the table workspace that will be created to store the investigation types list");
		}
		/// exec method
		void CListInvestigationTypes::exec()
		{
			ITableWorkspace_sptr ws_sptr= WorkspaceFactory::Instance().createTable("TableWorkspace");
			listInvestigationTypes(ws_sptr);
			setProperty("OutputWorkspace",ws_sptr);
		}
		/**This method returns list  of investigation types  from ICat  
		 *@param ws_sptr - shared pointer to table workspace
		 */
		void CListInvestigationTypes::listInvestigationTypes(ITableWorkspace_sptr& ws_sptr)
		{
			CSearchHelper searchObj;
			searchObj.listInvestigationTypes(ws_sptr);
		}

	}
}
