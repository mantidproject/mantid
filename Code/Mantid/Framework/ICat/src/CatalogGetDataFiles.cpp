/*WIKI*

This algorithm retrieves the files associated to selected investigation from the information catalog and saves the file search results to mantid workspace.

*WIKI*/

#include "MantidICat/CatalogGetDataFiles.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidKernel/MandatoryValidator.h"
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
      declareProperty("InvestigationId","",boost::make_shared<Kernel::MandatoryValidator<std::string>>(),
          "ID of the selected investigation");
      declareProperty(new API::WorkspaceProperty<API::ITableWorkspace> ("OutputWorkspace", "", Kernel::Direction::Output),
          "The name of the workspace to store the data file search details");
    }

    //execute the algorithm
    void CatalogGetDataFiles::exec()
    {
      auto workspace = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
      CatalogAlgorithmHelper().createCatalog()->getDataFiles(getProperty("InvestigationId"),workspace);
      setProperty("OutputWorkspace",workspace);
    }

  }
}
