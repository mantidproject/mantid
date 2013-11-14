/*WIKI*

This algorithm retrieves the investigation types from the information
catalog and saves investigation types lists to a mantid internal data structure.

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
      declareProperty("IsValid",true,"Boolean option used to check the validity of login session", Kernel::Direction::Output);
    }

    /// exec method
    void CatalogListInvestigationTypes::exec()
    {
      API::ICatalog_sptr catalog = CatalogAlgorithmHelper().createCatalog();
      std::vector<std::string> investTypes;
      catalog->listInvestigationTypes(investTypes);
      setProperty("InvestigationTypes",investTypes);
    }

  }
}
