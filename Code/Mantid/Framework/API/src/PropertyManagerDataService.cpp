#include "MantidAPI/PropertyManagerDataService.h"

namespace Mantid {
namespace API {
/**
* Default constructor
*/
PropertyManagerDataServiceImpl::PropertyManagerDataServiceImpl()
    : Mantid::Kernel::DataService<Mantid::Kernel::PropertyManager>(
          "PropertyManagerDataService") {}

/*
* Destructor
*/
PropertyManagerDataServiceImpl::~PropertyManagerDataServiceImpl() {}

} // Namespace API
} // Namespace Mantid
