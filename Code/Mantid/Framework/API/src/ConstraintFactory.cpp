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

    IConstraint* ConstraintFactoryImpl::createInitialized(IFunction* fun, const std::string& input, bool isDefault) const
    {
      Expression expr;
      expr.parse(input);
      return createInitialized(fun,expr,isDefault);
    }

    IConstraint* ConstraintFactoryImpl::createInitialized(IFunction* fun, const Expression& expr, bool isDefault) const
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
      c->initialize(fun,expr,isDefault);
      return c;
    }

  } // namespace API
} // namespace Mantid
