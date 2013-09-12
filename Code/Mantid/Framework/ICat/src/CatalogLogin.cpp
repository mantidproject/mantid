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
      auto requireValue = boost::make_shared<Kernel::MandatoryValidator<std::string> >();
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
        g_log.information() << "Attempting to login to " << ConfigService::Instance().getFacility().catalogName()
                            << " for " << ConfigService::Instance().getFacility().name() << "\n";
        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogName());

      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throwCatalogError();
      }
      if(!catalog_sptr)
      {
        throwCatalogError();
      }
      g_log.notice() << "Verifying user credentials..." << std::endl;
      progress(0.5, "Verifying user credentials...");
      catalog_sptr->login(username,password,"");

    }

    /// Raise an error concerning catalog searching
    void CatalogLogin::throwCatalogError() const
    {
      const std::string facilityName = ConfigService::Instance().getFacility().name();
      std::stringstream ss;
      ss << "Your current Facility, " << facilityName << ", does not have ICAT catalog information. "
          << std::endl;
      ss << "The facilities.xml file may need updating. Contact the Mantid Team for help." << std::endl;
      throw std::runtime_error(ss.str());
    }


  }
}

