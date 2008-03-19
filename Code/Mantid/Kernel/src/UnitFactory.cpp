#include "MantidKernel/UnitFactory.h"

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
