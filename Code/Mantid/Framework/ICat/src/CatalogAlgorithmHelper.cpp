#include "MantidICat/CatalogAlgorithmHelper.h"

namespace Mantid
{
  namespace ICat
  {

    /**
     * Create a catalog to use in the algorithms.
     * @return A pointer to the catalog class.
     */
    API::ICatalog_sptr CatalogAlgorithmHelper::createCatalog()
    {
      API::ICatalog_sptr catalog;
      try
      {
        catalog = API::CatalogFactory::Instance().create(Kernel::ConfigService::Instance().getFacility().catalogInfo().catalogName());
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw std::runtime_error("Your current Facility: " + Kernel::ConfigService::Instance().getFacility().name() + " does not have catalog information.\n");
      }
      return catalog;
    }
  }
}
