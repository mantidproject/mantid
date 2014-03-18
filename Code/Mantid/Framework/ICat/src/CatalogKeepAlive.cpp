/*WIKI*

This algorithm refreshes the current session to the maximum amount provided by the catalog API.

*WIKI*/

#include "MantidICat/CatalogKeepAlive.h"
#include "MantidAPI/CatalogManager.h"

#include <Poco/Thread.h>

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
      declareProperty<long>("TimePeriod",1200000,"Frequency to refresh session in milliseconds. Default 1200000 (20 minutes).",
          Kernel::Direction::Input);
    }

    void CatalogKeepAlive::exec()
    {
      long timePeriod = getProperty("TimePeriod");
      if (timePeriod <= 0)
        throw std::runtime_error("TimePeriod must be greater than zero.");

      // Keep going until cancelled
      while (true)
      {
        // Exit if the user presses cancel
        this->interruption_point();
        Poco::Thread::sleep(timePeriod);
        API::CatalogManager::Instance().getCatalog(getPropertyValue("Session"))->keepAlive();
      }
    }

  }
}

