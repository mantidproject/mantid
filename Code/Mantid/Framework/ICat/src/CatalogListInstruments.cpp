/*WIKI*
This algorithm retrieves the instrument names from the information
catalog and saves instrument lists to a mantid internal data structure.

*WIKI*/

#include "MantidICat/CatalogListInstruments.h"
#include "MantidICat/CatalogAlgorithmHelper.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
  namespace ICat
  {

    DECLARE_ALGORITHM(CatalogListInstruments)

    /// Sets documentation strings for this algorithm
    void CatalogListInstruments::initDocs()
    {
      this->setWikiSummary("Lists the name of instruments from Information catalog. ");
      this->setOptionalMessage("Lists the name of instruments from Information catalog.");
    }

    /// Init method
    void CatalogListInstruments::init()
    {
      declareProperty( new Kernel::ArrayProperty<std::string>("InstrumentList",std::vector<std::string>(),
                                                      boost::make_shared<Kernel::NullValidator>(),
                                                      Kernel::Direction::Output),
                       "A list containing instrument names");
      declareProperty("IsValid",true,"Boolean option used to check the validity of login session", Kernel::Direction::Output);
    }

    /// exec method
    void CatalogListInstruments::exec()
    {
      std::vector<std::string> intruments;
      CatalogAlgorithmHelper().createCatalog()->listInstruments(intruments);
      setProperty("InstrumentList",intruments);
    }

  }
}

