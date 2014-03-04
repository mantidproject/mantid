#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/ICostFunction.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"
#include <iostream>

namespace Mantid
{
  namespace API
  {

    CostFunctionFactoryImpl::CostFunctionFactoryImpl() : Kernel::DynamicFactory<ICostFunction>(), g_log(Kernel::Logger::get("CostFunctionFactory"))
    {
      // we need to make sure the library manager has been loaded before we 
      // are constructed so that it is destroyed after us and thus does
      // not close any loaded DLLs with loaded algorithms in them
      Mantid::Kernel::LibraryManager::Instance();
      g_log.debug() << "CostFunctionFactory created." << std::endl;
    }

  } // namespace API
} // namespace Mantid
