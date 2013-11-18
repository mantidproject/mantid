/*WIKI*
This algorithm retrieves the datasets associated to the selected investigation
from the information catalog and saves the search results to mantid workspace.
*WIKI*/

#include "MantidICat/CatalogGetDataSets.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"

namespace Mantid
{
  namespace ICat
  {
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
      auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int64_t> >();
      mustBePositive->setLower(0);
      declareProperty<int64_t>("InvestigationId",-1,mustBePositive,"Id of the selected investigation");
      declareProperty(new API::WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Kernel::Direction::Output),
          "The name of the workspace to store the result of datasets search ");
    }

    /// exec methods
    void CatalogGetDataSets::exec()
    {
      API::ICatalog_sptr catalog = CatalogAlgorithmHelper().createCatalog();
      API::ITableWorkspace_sptr ws_sptr = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      int64_t investigationId = getProperty("InvestigationId");
      catalog->getDataSets(investigationId,ws_sptr);
      setProperty("OutputWorkspace",ws_sptr);
    }

  }
}
