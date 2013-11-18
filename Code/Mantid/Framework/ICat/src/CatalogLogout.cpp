/*WIKI*

This algorithm disconnects the logged in user from the information catalog.

*WIKI*/

#include "MantidICat/CatalogLogout.h"
#include "MantidICat/CatalogAlgorithmHelper.h"

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogLogout)

    /// Sets documentation strings for this algorithm
    void CatalogLogout::initDocs()
    {
      this->setWikiSummary("Disconnects from information catalog.");
      this->setOptionalMessage("Disconnects from information catalog.");
    }

    /// Init method to declare algorithm properties
    void CatalogLogout::init() {}

    /// execute the algorithm
    void CatalogLogout::exec()
    {
      API::ICatalog_sptr catalog = CatalogAlgorithmHelper().createCatalog();
      catalog->logout();
    }
  }
}
