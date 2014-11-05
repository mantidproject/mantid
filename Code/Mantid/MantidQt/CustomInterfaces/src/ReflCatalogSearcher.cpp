#include "MantidQtCustomInterfaces/ReflCatalogSearcher.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"

using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    ITableWorkspace_sptr ReflCatalogSearcher::search(const std::string& text, const std::string& instrument)
    {
      //Currently unused
      (void)instrument;

      auto sessions = CatalogManager::Instance().getActiveSessions();
      if(sessions.empty())
        throw std::runtime_error("You are not logged into any catalogs.");

      const std::string sessionId = sessions.front()->getSessionId();

      auto algSearch = AlgorithmManager::Instance().create("CatalogGetDataFiles");
      algSearch->initialize();
      algSearch->setChild(true);
      algSearch->setLogging(false);
      algSearch->setProperty("Session", sessionId);
      algSearch->setProperty("InvestigationId", text);
      algSearch->setProperty("OutputWorkspace", "_ReflSearchResults");
      algSearch->execute();
      auto results = algSearch->getProperty("OutputWorkspace");

      //TODO: Postprocess results (remove non .raw files, etc.)

      return results;
    }
  }
}
