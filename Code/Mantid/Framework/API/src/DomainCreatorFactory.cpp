// Includes
#include "MantidAPI/DomainCreatorFactory.h"

namespace Mantid
{
  namespace API
  {
    //----------------------------------------------------------------------------------------------
    // Private methods
    //----------------------------------------------------------------------------------------------

    /**
     * Constructor
     */
    DomainCreatorFactoryImpl::DomainCreatorFactoryImpl()
      : Kernel::DynamicFactory<IDomainCreator>()
    {
    }

    /**
     * Destructor
     */
    DomainCreatorFactoryImpl::~DomainCreatorFactoryImpl()
    {
    }

  } // namespace API
} // namespace Mantid
