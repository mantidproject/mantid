/*WIKI*

This algorithm retrieves the files associated to selected investigation from the information catalog and saves the file search results to mantid workspace.

*WIKI*/

#include "MantidICat/CatalogGetDataFiles.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/WorkspaceProperty.h"

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogGetDataFiles)

    /// Sets documentation strings for this algorithm
    void CatalogGetDataFiles::initDocs()
    {
      this->setWikiSummary("Gets the files associated to the selected investigation.");
      this->setOptionalMessage("Gets the files associated to the selected investigation.");
    }

    /// Initialising the algorithm
    void CatalogGetDataFiles::init()
    {
      auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int64_t>>();
      mustBePositive->setLower(0);
      declareProperty<int64_t>("InvestigationId",-1,mustBePositive,"ID of the selected investigation");

      declareProperty(new API::WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Kernel::Direction::Output),
          "The name of the workspace to store the data file search details");
    }

    //execute the algorithm
    void CatalogGetDataFiles::exec()
    {
      API::ICatalog_sptr catalog = CatalogAlgorithmHelper().createCatalog();

      int64_t investigationId  = getProperty("InvestigationId");

      API::ITableWorkspace_sptr ws_sptr = API::WorkspaceFactory::Instance().createTable("TableWorkspace");

      catalog->getDataFiles(investigationId,ws_sptr);

      setProperty("OutputWorkspace",ws_sptr);
    }

  }
}
