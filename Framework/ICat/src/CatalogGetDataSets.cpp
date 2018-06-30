#include "MantidICat/CatalogGetDataSets.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogGetDataSets)

/// Initialisation methods
void CatalogGetDataSets::init() {
  declareProperty("InvestigationId", "",
                  boost::make_shared<Kernel::MandatoryValidator<std::string>>(),
                  "ID of the selected investigation");
  declareProperty("Session", "",
                  "The session information of the catalog to use.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the workspace to store the results.");
}

/// exec methods
void CatalogGetDataSets::exec() {
  auto workspace =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  API::CatalogManager::Instance()
      .getCatalog(getPropertyValue("Session"))
      ->getDataSets(getProperty("InvestigationId"), workspace);
  setProperty("OutputWorkspace", workspace);
}
} // namespace ICat
} // namespace Mantid
