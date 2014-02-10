#include "MantidSINQ/PoldiChopperFactory.h"

#include "MantidSINQ/PoldiBasicChopper.h"

namespace Mantid
{
namespace Poldi
{

Poldi::PoldiAbstractChopper *PoldiChopperFactory::createChopper(std::string chopperType)
{
    UNUSED_ARG(chopperType);

    return new PoldiBasicChopper();
}

}
// namespace Poldi
} // namespace Mantid
