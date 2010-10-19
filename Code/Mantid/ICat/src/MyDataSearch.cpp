#include "MantidICat/MyDataSearch.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"

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

						
			ICatalog_sptr catalog_sptr;
			try
			{			
			 catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().Facility().catalogName());
			
			}
			catch(Kernel::Exception::NotFoundError&)
			{
				throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
			} 
			if(!catalog_sptr)
			{
				throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
			}
			
			API::ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable("TableWorkspace");
			catalog_sptr->myData(outputws);
			setProperty("OutputWorkspace",outputws);
		
		}
		
	}
}
