/*WIKI*
This algorithm retrieves the instrument names from the information
catalog and saves instrument lists to a mantid internal data structure.

*WIKI*/

#include "MantidICat/CatalogListInstruments.h"
#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
  namespace ICat
  {
    using namespace Kernel;
    using namespace API;

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
      declareProperty( new ArrayProperty<std::string>("InstrumentList",std::vector<std::string>(),
                                                      boost::make_shared<NullValidator>(),
                                                      Direction::Output),
                       "A list containing instrument names");
      declareProperty("IsValid",true,"Boolean option used to check the validity of login session", Direction::Output);
    }

    /// exec method
    void CatalogListInstruments::exec()
    {
      ICatalog_sptr catalog_sptr;
      try
      {

        catalog_sptr=CatalogFactory::Instance().create(ConfigService::Instance().getFacility().catalogName());

      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file.");
      }
      if(!catalog_sptr)
      {
        throw std::runtime_error("Error when getting the catalog information from the Facilities.xml file");
      }
      std::vector<std::string> intruments;
      try
      {
        catalog_sptr->listInstruments(intruments);
      }
      catch(std::runtime_error& e )
      {
        setProperty("IsValid",false);
        throw std::runtime_error("Please login to the information catalog using the login dialog provided.");
      }
      setProperty("InstrumentList",intruments);
    }


  }
}

