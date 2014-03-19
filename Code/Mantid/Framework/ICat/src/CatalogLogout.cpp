/*WIKI*

This algorithm disconnects the logged in user from a specific catalog using the session information provided.

*WIKI*/

#include "MantidICat/CatalogLogout.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/AlgorithmProperty.h"

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogLogout)

    /// Sets documentation strings for this algorithm
    void CatalogLogout::initDocs()
    {
      this->setWikiSummary("Logs out of a specific catalog using the session information provided.");
      this->setOptionalMessage("Logs out of a specific catalog using the session information provided.");
    }

    /// Init method to declare algorithm properties
    void CatalogLogout::init()
    {
      declareProperty("Session",std::string(""),
          "The session information of the catalog to log out. If none provided then all catalogs are logged out.",
          Kernel::Direction::Input);
    }

    /// execute the algorithm
    void CatalogLogout::exec()
    {
      API::CatalogManager::Instance().destroyCatalog(getPropertyValue("Session"));
    }
  }
}
