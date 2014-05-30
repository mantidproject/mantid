/*WIKI*

This algorithm retrieves logged in users investigations data from the information catalog and stores it in mantid workspace.

*WIKI*/

#include "MantidICat/CatalogMyDataSearch.h"
#include "MantidAPI/CatalogManager.h"

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogMyDataSearch)

    /// Initialisation method.
    void CatalogMyDataSearch::init()
    {
      declareProperty("Session","","The session information of the catalog to use.");
      declareProperty(new API::WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Kernel::Direction::Output),
          "The name of the workspace to store the search results.");
    }

    /// Execution method.
    void CatalogMyDataSearch::exec()
    {
      auto outputws = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      API::CatalogManager::Instance().getCatalog(getPropertyValue("Session"))->myData(outputws);
      setProperty("OutputWorkspace",outputws);
    }
  }
}
