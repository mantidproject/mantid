#include "MantidICat/ListInvestigationTypes.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidICat/ErrorHandling.h"
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
			declareProperty( new ArrayProperty<std::string>("InvestigationTypes",std::vector<std::string>(),new NullValidator<std::vector<std::string>>, 
				Direction::Output),"List of investigation types obtained from Catalog");
			declareProperty("isValid",true,"Boolean option used to check the validity of login session", Direction::Output);
		}
		/// exec method
		void CListInvestigationTypes::exec()
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
			std::vector<std::string> investTypes;
			try
			{
			catalog_sptr->listInvestigationTypes(investTypes);
			}
			catch(SessionException& )
			{			   
				setProperty("isValid",false);
				throw std::runtime_error("Invalid Session");
			}
			setProperty("InvestigationTypes",investTypes);
		}
		
	}
}
