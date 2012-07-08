#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Kernel
{

UnitFactoryImpl::UnitFactoryImpl() :
  DynamicFactory<Unit>(), m_log(Kernel::Logger::get("UnitFactory"))
{
}

UnitFactoryImpl::~UnitFactoryImpl()
{
}

} // namespace Kernel
} // namespace Mantid
