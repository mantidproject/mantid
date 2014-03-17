/*WIKI*

This algorithm refreshes the current session to the maximum amount provided by the catalog API.

*WIKI*/

#include "MantidICat/CatalogKeepAlive.h"
#include "MantidAPI/CatalogManager.h"

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogKeepAlive)

    void CatalogKeepAlive::initDocs()
    {
      this->setWikiSummary("Refreshes the current session to the maximum amount provided by the catalog API");
      this->setOptionalMessage("Refreshes the current session to the maximum amount provided by the catalog API");
    }

    void CatalogKeepAlive::init()
    {
      declareProperty("Session","","The session information of the catalog to use.");
    }

    void CatalogKeepAlive::exec()
    {
      API::CatalogManager::Instance().getCatalog(getPropertyValue("Session"))->keepAlive();
    }

  }
}

