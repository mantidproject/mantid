/*WIKI*

This algorithm disconnects the logged in user from the information catalog.

*WIKI*/

#include "MantidICat/CatalogLogout.h"
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
    DECLARE_ALGORITHM(CatalogLogout)

    /// Sets documentation strings for this algorithm
    void CatalogLogout::initDocs()
    {
      this->setWikiSummary("Disconnects from information catalog.");
      this->setOptionalMessage("Disconnects from information catalog.");
    }

    /// Init method to declare algorithm properties
    void CatalogLogout::init()
    {
    }

    /// execute the algorithm
    void CatalogLogout::exec()
    {
      ICatalog_sptr catalog_sptr;
      try
      {
        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogInfo().catalogName());
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
      }
      if(!catalog_sptr)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
      }
      catalog_sptr->logout();

    }

  }
}
