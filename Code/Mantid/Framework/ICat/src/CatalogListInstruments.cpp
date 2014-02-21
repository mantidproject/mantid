/*WIKI*

This algorithm retrieves the instrument names from a catalog and stores them in a vector.

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

