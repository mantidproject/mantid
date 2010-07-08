#include "MantidICat/SearchByRunNumber.h"
#include"MantidICat/Session.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include"MantidICat/SearchHelper.h"
#include <iomanip>
namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CSearchByRunNumber)
		/// Initialisation method.
		void CSearchByRunNumber::init()
		{
			BoundedValidator<double>* mustBePositive = new BoundedValidator<double>();
			mustBePositive->setLower(0.0);
			declareProperty("StartRun",0.0,mustBePositive,"The start run number");
			declareProperty("EndRun",0.0,mustBePositive->clone(),"The end run number");
			
			declareProperty("Instruments","",new Kernel::MandatoryValidator<std::string>(),
				"The list of instruments used in ISIS nuetron scattering experiments");

			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
				"The name of the workspace that will be created to store the search result");
		}
		/// Execution method.
		void CSearchByRunNumber::exec()
		{			
			API::ITableWorkspace_sptr ws_sptr=doSearchByRunNumber();
			setProperty("OutputWorkspace",ws_sptr);
		}

		/* This method does search by run number and instrument name.
		 * @returns shared pointer to table workspace which stores the data
		 */
		API::ITableWorkspace_sptr  CSearchByRunNumber::doSearchByRunNumber()
		{	
			double dstartRun=getProperty("StartRun");
			double dendRun=getProperty("EndRun");
			std::string instrument = getPropertyValue("Instruments");
		
			API::ITableWorkspace_sptr outputws;
			CSearchHelper searchobj;
			int ret_advsearch=searchobj.doSearchByRunNumber(dstartRun,dendRun,instrument,ns1__investigationInclude__DATASETS_USCOREAND_USCOREDATAFILES,outputws);
			return outputws;
		
		}

	
	}
}

