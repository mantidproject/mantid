#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IConstraint.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/LibraryManager.h"
#include <Poco/StringTokenizer.h>

namespace Mantid
{
  namespace API
  {

    ConstraintFactoryImpl::ConstraintFactoryImpl() : Kernel::DynamicFactory<IConstraint>(), g_log(Kernel::Logger::get("ConstraintFactory"))
    {
      // we need to make sure the library manager has been loaded before we 
      // are constructed so that it is destroyed after us and thus does
      // not close any loaded DLLs with loaded algorithms in them
      Mantid::Kernel::LibraryManager::Instance();
      g_log.debug() << "ConstraintFactory created." << std::endl;
    }

    ConstraintFactoryImpl::~ConstraintFactoryImpl()
    {
    }

    IConstraint* ConstraintFactoryImpl::createInitialized(IFitFunction* fun, const std::string& input) const
    {
      Expression expr;
      expr.parse(input);
      return createInitialized(fun,expr);
    }

    IConstraint* ConstraintFactoryImpl::createInitialized(IFitFunction* fun, const Expression& expr) const
    {
      IConstraint* c = 0;
      if (expr.name() == "==")
      {
        c = createUnwrapped("BoundaryConstraint");
      }
      else
      {
        c = createUnwrapped(expr.name());
      }
      c->initialize(fun,expr);
      return c;
    }

  } // namespace API
} // namespace Mantid
