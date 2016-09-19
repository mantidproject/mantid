#include "MantidKernel/PropertyManagerDataService.h"

namespace Mantid {
namespace Kernel {

/**
 * Default constructor
 */
PropertyManagerDataServiceImpl::PropertyManagerDataServiceImpl()
    : DataService<PropertyManager>("PropertyManagerDataService") {}

} // Namespace Kernel
} // Namespace Mantid
