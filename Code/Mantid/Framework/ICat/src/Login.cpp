#include "MantidICat/Login.h"
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

    DECLARE_ALGORITHM(Login)

    /// Sets documentation strings for this algorithm
    void Login::initDocs()
    {
      this->setWikiSummary("Connects to information catalog using user name and password. ");
      this->setOptionalMessage("Connects to information catalog using user name and password.");
    }


    /// Init method to declare algorithm properties
    void Login::init()
    {
      declareProperty("Username","", new Kernel::MandatoryValidator<std::string>(),
          "The name of the logged in user");
      declareProperty(new MaskedProperty<std::string>("Password","",new Kernel::MandatoryValidator<std::string>()),
          "The password of the logged in user ");

      //declareProperty("DBServer","","Parameter that will identify the ICat DB server URL");
    }
    /// execute the algorithm
    void Login::exec()
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

