#include "MantidKernel/PropertyManagerDataService.h"

namespace Mantid {
namespace Kernel {

/**
 * Default constructor
 */
PropertyManagerDataServiceImpl::PropertyManagerDataServiceImpl()
    : DataService<PropertyManager>("PropertyManagerDataService") {}

template class Mantid::Kernel::SingletonHolder<PropertyManagerDataServiceImpl>;

} // Namespace Kernel
} // Namespace Mantid
