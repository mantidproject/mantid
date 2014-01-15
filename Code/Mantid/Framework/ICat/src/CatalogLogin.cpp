/*WIKI*

This algorithm connects the logged in user to the information catalog.

*WIKI*/

#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogLogin)

    /// Sets documentation strings for this algorithm
    void CatalogLogin::initDocs()
    {
      this->setWikiSummary("Connects to information catalog using user name and password.");
      this->setOptionalMessage("Connects to information catalog using user name and password.");
    }

    /// Init method to declare algorithm properties
    void CatalogLogin::init()
    {
      auto requireValue = boost::make_shared<Kernel::MandatoryValidator<std::string>>();
      declareProperty("Username","", requireValue,"The username to log into the catalog.");
      declareProperty(new Kernel::MaskedProperty<std::string>("Password","", requireValue),
                      "The password of the related username to use.");
      declareProperty("FacilityName",Mantid::Kernel::ConfigService::Instance().getFacility().name(),
    		  boost::make_shared<Kernel::StringListValidator>(Kernel::ConfigService::Instance().getFacilityNames()),
    		  "Select a facility to log in to.");
    }

    /// execute the algorithm
    void CatalogLogin::exec()
    {
      // Obtain the soapEndPoint based on the name of the facility the user has selected.
      std::string soapEndPoint = Kernel::ConfigService::Instance().getFacility(getProperty("FacilityName")).catalogInfo().soapEndPoint();
      if (soapEndPoint.empty()) throw std::runtime_error("There is no soap end-point for the facility you have selected.");

      g_log.notice() << "Attempting to verify user credentials against " <<
          Mantid::Kernel::ConfigService::Instance().getFacility().catalogInfo().catalogName() << std::endl;
      progress(0.5, "Verifying user credentials...");
      CatalogAlgorithmHelper().createCatalog()->login(getProperty("Username"), getProperty("Password"), soapEndPoint);
    }

  }
}

