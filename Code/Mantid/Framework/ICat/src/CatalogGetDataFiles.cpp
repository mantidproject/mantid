#include "MantidICat/CatalogGetDataFiles.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/WorkspaceProperty.h"

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogGetDataFiles)

/// Initialising the algorithm
void CatalogGetDataFiles::init() {
  declareProperty("InvestigationId", "",
                  boost::make_shared<Kernel::MandatoryValidator<std::string>>(),
                  "ID of the selected investigation");
  declareProperty("Session", "",
                  "The session information of the catalog to use.");
  declareProperty(
      new API::WorkspaceProperty<API::ITableWorkspace>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the workspace to store the data file search details.");
}

// execute the algorithm
void CatalogGetDataFiles::exec() {
  auto workspace =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  API::CatalogManager::Instance()
      .getCatalog(getPropertyValue("Session"))
      ->getDataFiles(getProperty("InvestigationId"), workspace);
  setProperty("OutputWorkspace", workspace);
}
}
}
