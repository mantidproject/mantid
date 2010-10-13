#include "MantidAPI/CatalogFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace API
{

/// Constructor
CatalogFactoryImpl::CatalogFactoryImpl() :
  Kernel::DynamicFactory<ICatalog>(), m_createdCatalogs(), m_log(Kernel::Logger::get("CatalogFactory"))
{
}

 /// Destructor
CatalogFactoryImpl::~CatalogFactoryImpl()
{
}

/** Returns an instance of the class with the given name. Overrides the base class method.
 *  If an instance already exists, a pointer to it is returned, otherwise
 *  a new instance is created by the DynamicFactory::create method.
 *  @param className The name of the class to be created
 *  @return A shared pointer to the instance of the requested unit
 */
boost::shared_ptr<ICatalog> CatalogFactoryImpl::create(const std::string& className) const
{
  std::map< std::string, boost::shared_ptr<ICatalog> >::const_iterator it = m_createdCatalogs.find(className);
  if ( it != m_createdCatalogs.end() )
  {
    // If an instance has previously been created, just return a pointer to it
    return it->second;
  }
  else
  {
    // Otherwise create & return a new instance and store the pointer in the internal map for next time
    return m_createdCatalogs[className] = Kernel::DynamicFactory<API::ICatalog>::create(className);
  }
}

} // namespace Kernel
} // namespace Mantid
