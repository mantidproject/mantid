#include "MantidAPI/InstrumentDataService.h"

namespace Mantid {
namespace API {
/**
* Default constructor
*/
InstrumentDataServiceImpl::InstrumentDataServiceImpl()
    : Mantid::Kernel::DataService<Mantid::Geometry::Instrument>(
          "InstrumentDataService") {}

} // Namespace API
} // Namespace Mantid

namespace Mantid {
namespace Kernel {
  template class Mantid::Kernel::SingletonHolder<Mantid::API::InstrumentDataServiceImpl>;
}
}

