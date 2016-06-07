//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ArchiveSearchFactory.h"

namespace Mantid {
namespace API {

/// Default constructor
ArchiveSearchFactoryImpl::ArchiveSearchFactoryImpl() {}
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
  template class Mantid::Kernel::SingletonHolder<Mantid::API::ArchiveSearchFactoryImpl>;
}
}
