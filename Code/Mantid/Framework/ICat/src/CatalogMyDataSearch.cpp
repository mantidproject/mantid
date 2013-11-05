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
      ICatalog_sptr catalog = CatalogAlgorithmHelper().createCatalog();

      API::ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable("TableWorkspace");
      try
      {
        catalog->myData(outputws);
      }
      catch(std::runtime_error&)
      {
        setProperty("IsValid",false);
        throw std::runtime_error("Please login to the information catalog using the login dialog provided.");
      }
      setProperty("OutputWorkspace",outputws);
    }
  }
}
