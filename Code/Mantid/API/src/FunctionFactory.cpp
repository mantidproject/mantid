#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"
namespace Mantid
{
  namespace API
  {

    FunctionFactoryImpl::FunctionFactoryImpl() : Kernel::DynamicFactory<IFunction>(), g_log(Kernel::Logger::get("FunctionFactory"))
    {
    }

    FunctionFactoryImpl::~FunctionFactoryImpl()
    {
    }

  } // namespace DataObjects
} // namespace Mantid
