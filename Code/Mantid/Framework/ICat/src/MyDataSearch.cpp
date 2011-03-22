#include "MantidICat/MyDataSearch.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"
#include "MantidICat/ErrorHandling.h"

namespace Mantid
{
	namespace ICat
	{
		using namespace Kernel;
		using namespace API;

		DECLARE_ALGORITHM(CMyDataSearch)

		/// Sets documentation strings for this algorithm
		void CMyDataSearch::initDocs()
		{
		  this->setWikiSummary("This algorithm  loads the logged in users investigations . ");
		  this->setOptionalMessage("This algorithm  loads the logged in users investigations .");
		}

		/// Initialisation method.
		void CMyDataSearch::init()
		{
			declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
                            "The name of the workspace to store the result of MyData search ");
      declareProperty("isValid",true,"Boolean option used to check the validity of login session", Direction::Output);
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
      try
      {
			catalog_sptr->myData(outputws);
      }
      catch(SessionException& e)
      {
        setProperty("isValid",false);
        throw std::runtime_error(e.what());
      }
			setProperty("OutputWorkspace",outputws);
		
		}
		
	}
}
