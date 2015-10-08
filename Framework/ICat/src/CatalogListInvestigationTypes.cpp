#include "MantidICat/CatalogListInvestigationTypes.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace ICat {
DECLARE_ALGORITHM(CatalogListInvestigationTypes)

/// Init method
void CatalogListInvestigationTypes::init() {
  declareProperty("Session", "",
                  "The session information of the catalog to use.");
  declareProperty(new Kernel::ArrayProperty<std::string>(
                      "InvestigationTypes", std::vector<std::string>(),
                      boost::make_shared<Kernel::NullValidator>(),
                      Kernel::Direction::Output),
                  "A list containing investigation types.");
}

/// exec method
void CatalogListInvestigationTypes::exec() {
  std::vector<std::string> investigationTypes;
  API::CatalogManager::Instance()
      .getCatalog(getPropertyValue("Session"))
      ->listInvestigationTypes(investigationTypes);
  setProperty("InvestigationTypes", investigationTypes);
}
}
}
