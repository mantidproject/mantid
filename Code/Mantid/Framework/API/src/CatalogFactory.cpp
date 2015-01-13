#include "MantidAPI/CatalogFactory.h"

namespace Mantid {
namespace API {
/// Constructor
CatalogFactoryImpl::CatalogFactoryImpl()
    : Kernel::DynamicFactory<ICatalog>(), m_createdCatalogs() {}
/// Destructor
CatalogFactoryImpl::~CatalogFactoryImpl() {}
}
}
