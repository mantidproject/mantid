#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
  namespace API
  {

    FuncMinimizerFactoryImpl::FuncMinimizerFactoryImpl() : Kernel::DynamicFactory<IFuncMinimizer>(), g_log(Kernel::Logger::get("FuncMinimizerFactory"))
    {
      // we need to make sure the library manager has been loaded before we 
      // are constructed so that it is destroyed after us and thus does
      // not close any loaded DLLs with loaded algorithms in them
      Mantid::Kernel::LibraryManager::Instance();
      g_log.debug() << "FuncMinimizerFactory created." << std::endl;
    }

  } // namespace API
} // namespace Mantid
