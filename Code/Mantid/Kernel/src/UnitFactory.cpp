#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Kernel
{

UnitFactoryImpl::UnitFactoryImpl() : Kernel::DynamicFactory<Unit>(), g_log(Kernel::Logger::get("UnitFactory"))
{
}

UnitFactoryImpl::~UnitFactoryImpl()
{
}

}
}
