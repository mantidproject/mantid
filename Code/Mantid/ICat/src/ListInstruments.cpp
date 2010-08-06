#include "MantidICat/ListInstruments.h"
#include "MantidICat/SearchHelper.h"
#include "MantidAPI/WorkspaceProperty.h"


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
			API::ITableWorkspace_sptr ws_sptr=listInstruments();
			setProperty("OutputWorkspace",ws_sptr);
		}
		/// List instruments
		API::ITableWorkspace_sptr CListInstruments::listInstruments()
		{
			CSearchHelper searchObj;
			return searchObj.listInstruments();
		}

	}
}

