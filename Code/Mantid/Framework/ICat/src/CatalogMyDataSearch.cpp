/*WIKI*

This algorithm retrieves logged in users investigations data from the information catalog and stores it in mantid workspace.

*WIKI*/

#include "MantidICat/CatalogMyDataSearch.h"
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


      ICatalog_sptr catalog_sptr;
      try
      {
        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogName());

      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
      }
      if(!catalog_sptr)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
      }

      API::ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable("TableWorkspace");
      try
      {
        catalog_sptr->myData(outputws);
      }
      catch(std::runtime_error& e)
      {
        setProperty("IsValid",false);
        throw std::runtime_error("Please login to the information catalog using the login dialog provided.");
      }
      setProperty("OutputWorkspace",outputws);

    }

  }
}
