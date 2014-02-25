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

} // namespace Kernel
} // namespace Mantid
