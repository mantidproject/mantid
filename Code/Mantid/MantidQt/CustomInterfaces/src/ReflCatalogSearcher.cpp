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
      ITableWorkspace_sptr results = algSearch->getProperty("OutputWorkspace");

      //Now, tidy up the data
      std::set<size_t> toRemove;
      for(size_t i = 0; i < results->rowCount(); ++i)
      {
        std::string& run = results->String(i,0);

        //Too short to be more than ".raw"
        if(run.size() < 5)
        {
          toRemove.insert(i);
        }
        //If this isn't the right instrument, remove it
        else if(run.substr(0, instrument.size()) != instrument)
        {
          toRemove.insert(i);
        }
        //If it's not a raw file, remove it
        else if(run.substr(run.size() - 4, 4) != ".raw")
        {
          toRemove.insert(i);
        }

        //It's a valid run, so let's trim the instrument prefix and ".raw" suffix
        run = run.substr(instrument.size(), run.size() - (instrument.size() + 4));

        //Let's also get rid of any leading zeros
        size_t numZeros = 0;
        while(run[numZeros] == '0')
          numZeros++;
        run = run.substr(numZeros, run.size() - numZeros);
      }

      //Sets are sorted so if we go from back to front we won't trip over ourselves
      for(auto row = toRemove.rbegin(); row != toRemove.rend(); ++row)
        results->removeRow(*row);

      return results;
    }
  }
}
