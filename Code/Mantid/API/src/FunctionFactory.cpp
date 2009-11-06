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

    IFunction* FunctionFactoryImpl::createFunction(const std::string& type) const
    {
      IFunction* fun = createUnwrapped(type);
      fun->initialize();
      return fun;
    }

  } // namespace API
} // namespace Mantid
