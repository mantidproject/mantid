#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace Kernel {

/**
 * Default constructor
 */
PropertyManagerDataServiceImpl::PropertyManagerDataServiceImpl()
    : DataService<PropertyManager>("PropertyManagerDataService") {}

} // Namespace Kernel
} // Namespace Mantid
