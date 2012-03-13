/*WIKI*

This algorithm connects the logged in user to the information catalog like ISIS ICat3Catalog, SNS VFS, etc.

*WIKI*/

#include "MantidICat/CatalogLogin.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"
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

    DECLARE_ALGORITHM(CatalogLogin)

    /// Sets documentation strings for this algorithm
    void CatalogLogin::initDocs()
    {
      this->setWikiSummary("Connects to information catalog using user name and password. ");
      this->setOptionalMessage("Connects to information catalog using user name and password.");
    }


    /// Init method to declare algorithm properties
    void CatalogLogin::init()
    {
      auto requireValue = boost::make_shared<Kernel::MandatoryValidator<std::string>>();
      declareProperty("Username","", requireValue,"The name/federal ID of the logged in user");
      declareProperty(new MaskedProperty<std::string>("Password","", requireValue),
                      "The password of the logged in user ");
    }
    /// execute the algorithm
    void CatalogLogin::exec()
    {
      std::string username=getProperty("Username");
      std::string password=getProperty("Password");
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
      catalog_sptr->login(username,password,"");

    }


  }
}

