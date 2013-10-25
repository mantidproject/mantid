/*WIKI*
This algorithm retrieves the datasets associated to the selected investigation
from the information catalog and saves the search results to mantid workspace.
*WIKI*/

#include "MantidICat/CatalogGetDataSets.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
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
    using std::size_t;

    DECLARE_ALGORITHM(CatalogGetDataSets)

    /// Sets documentation strings for this algorithm
    void CatalogGetDataSets::initDocs()
    {
      this->setWikiSummary("Gets the datasets associated to the selected investigation. ");
      this->setOptionalMessage("Gets the datasets associated to the selected investigation.");
    }

    /// Initialisation methods
    void CatalogGetDataSets::init()
    {
      auto mustBePositive = boost::make_shared<BoundedValidator<int64_t> >();
      mustBePositive->setLower(0);
      declareProperty<int64_t>("InvestigationId",-1,mustBePositive,"Id of the selected investigation");
      declareProperty(new WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Direction::Output),
          "The name of the workspace to store the result of datasets search ");
    }

    /// exec methods
    void CatalogGetDataSets::exec()
    {
      ICatalog_sptr catalog_sptr;
      try
      {
        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogInfo().catalogName());
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
      }
      if(!catalog_sptr)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
      }

      API::ITableWorkspace_sptr ws_sptr = WorkspaceFactory::Instance().createTable("TableWorkspace");
      int64_t investigationId = getProperty("InvestigationId");
      catalog_sptr->getDataSets(investigationId,ws_sptr);
      setProperty("OutputWorkspace",ws_sptr);
    }

  }
}
