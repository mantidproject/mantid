/*WIKI*

This algorithm retrieves the investigation types from the information
catalog and saves investigation types lists to a mantid internal data structure.

*WIKI*/

#include "MantidICat/CatalogListInvestigationTypes.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
  namespace ICat
  {
    using namespace Kernel;
    using namespace API;

    DECLARE_ALGORITHM(CatalogListInvestigationTypes)

    /// Sets documentation strings for this algorithm
    void CatalogListInvestigationTypes::initDocs()
    {
      this->setWikiSummary("Lists the name of investigation types from the Information catalog. ");
      this->setOptionalMessage("Lists the name of investigation types from the Information catalog.");
    }

    /// Init method
    void CatalogListInvestigationTypes::init()
    {
      declareProperty( new ArrayProperty<std::string>("InvestigationTypes",std::vector<std::string>(),
                                                      boost::make_shared<NullValidator>(),
                                                      Direction::Output),
                       "List of investigation types obtained from Catalog");
      declareProperty("IsValid",true,"Boolean option used to check the validity of login session", Direction::Output);
    }

    /// exec method
    void CatalogListInvestigationTypes::exec()
    {
      ICatalog_sptr catalog_sptr;
      try
      {
        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogName());
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
      catch(std::runtime_error& e)
      {
        setProperty("IsValid",false);
        throw std::runtime_error("Please login to the information catalog using the login dialog provided.");
      }
      setProperty("InvestigationTypes",investTypes);
    }

  }
}
