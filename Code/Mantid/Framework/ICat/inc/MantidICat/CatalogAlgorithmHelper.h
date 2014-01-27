#ifndef MANTID_CATALOGALGORITHMHELPER_H_
#define MANTID_CATALOGALGORITHMHELPER_H_

#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidAPI/ICatalog.h"

namespace Mantid
{
  namespace ICat
  {
    class CatalogAlgorithmHelper
    {
      public:
        /// Create a catalog to use in the algorithms.
        API::ICatalog_sptr createCatalog();
    };
  }
}

#endif
