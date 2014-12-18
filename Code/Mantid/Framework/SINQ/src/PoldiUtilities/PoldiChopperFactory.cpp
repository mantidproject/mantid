#include "MantidSINQ/PoldiUtilities/PoldiChopperFactory.h"

#include "MantidSINQ/PoldiUtilities/PoldiBasicChopper.h"

namespace Mantid {
namespace Poldi {

Poldi::PoldiAbstractChopper *
PoldiChopperFactory::createChopper(std::string chopperType) {
  UNUSED_ARG(chopperType);

  return new PoldiBasicChopper();
}
}
// namespace Poldi
} // namespace Mantid
