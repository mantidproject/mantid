/*WIKI*

This algorithm is responsible for obtaining a list of investigation types from the catalog.

*WIKI*/

#include "MantidICat/CatalogListInvestigationTypes.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
  namespace ICat
  {
    DECLARE_ALGORITHM(CatalogListInvestigationTypes)

    /// Sets documentation strings for this algorithm
    void CatalogListInvestigationTypes::initDocs()
    {
      this->setWikiSummary("Lists the name of investigation types from the Information catalog. ");
      this->setOptionalMessage("Lists the name of investigation types from the Information catalog.");
    }

    /// Init method
    void CatalogListInvestigationTypes::init()
    {
      declareProperty( new Kernel::ArrayProperty<std::string>("InvestigationTypes",std::vector<std::string>(),
                                                      boost::make_shared<Kernel::NullValidator>(),
                                                      Kernel::Direction::Output),
                       "List of investigation types obtained from Catalog");
    }

    /// exec method
    void CatalogListInvestigationTypes::exec()
    {
      std::vector<std::string> investTypes;
      CatalogAlgorithmHelper().createCatalog()->listInvestigationTypes(investTypes);
      setProperty("InvestigationTypes",investTypes);
    }

  }
}
