#include "MantidCurveFitting/CostFunctionFactory.h"
#include "MantidCurveFitting/ICostFunction.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
  namespace CurveFitting
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
