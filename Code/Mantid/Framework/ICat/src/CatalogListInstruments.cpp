#include "MantidICat/CatalogListInstruments.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace ICat {

DECLARE_ALGORITHM(CatalogListInstruments)

/// Init method
void CatalogListInstruments::init() {
  declareProperty("Session", "",
                  "The session information of the catalog to use.");
  declareProperty(new Kernel::ArrayProperty<std::string>(
                      "InstrumentList", std::vector<std::string>(),
                      boost::make_shared<Kernel::NullValidator>(),
                      Kernel::Direction::Output),
                  "A list containing instrument names.");
}

/// exec method
void CatalogListInstruments::exec() {
  std::vector<std::string> instruments;
  API::CatalogManager::Instance()
      .getCatalog(getPropertyValue("Session"))
      ->listInstruments(instruments);
  setProperty("InstrumentList", instruments);
}
}
}
