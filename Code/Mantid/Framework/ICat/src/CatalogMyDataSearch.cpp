/*WIKI*

This algorithm retrieves logged in users investigations data from the information catalog and stores it in mantid workspace.

*WIKI*/

#include "MantidICat/CatalogMyDataSearch.h"
#include "MantidICat/CatalogAlgorithmHelper.h"

namespace Mantid
{
  namespace ICat
  {
    using namespace Kernel;
    using namespace API;

    DECLARE_ALGORITHM(CatalogMyDataSearch)

    /// Sets documentation strings for this algorithm
    void CatalogMyDataSearch::initDocs()
    {
      this->setWikiSummary("This algorithm loads the logged in users' investigations.");
      this->setOptionalMessage("This algorithm loads the logged in users' investigations.");
    }

    /// Initialisation method.
    void CatalogMyDataSearch::init()
    {
      declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
          "The name of the workspace to store the result of MyData search ");
      declareProperty("IsValid",true,"Boolean option used to check the validity of login session", Direction::Output);
    }

    /// Execution method.
    void CatalogMyDataSearch::exec()
    {
      // Create and use the catalog the user has specified in Facilities.xml
      auto outputws = WorkspaceFactory::Instance().createTable("TableWorkspace");
      CatalogAlgorithmHelper().createCatalog()->myData(outputws);
      setProperty("OutputWorkspace",outputws);
    }
  }
}
