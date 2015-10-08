#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IConstraint.h"
#include "MantidKernel/LibraryManager.h"
#include <Poco/StringTokenizer.h>

namespace Mantid {
namespace API {

ConstraintFactoryImpl::ConstraintFactoryImpl()
    : Kernel::DynamicFactory<IConstraint>() {
  // we need to make sure the library manager has been loaded before we
  // are constructed so that it is destroyed after us and thus does
  // not close any loaded DLLs with loaded algorithms in them
  Mantid::Kernel::LibraryManager::Instance();
}

ConstraintFactoryImpl::~ConstraintFactoryImpl() {}

/**Creates an instance of a Constraint initialized using an expression
 * @param fun :: The function
 * @param input :: The creation expression, format depends on the constraint
 * implementation. For BoundaryConstraint
 *   it is an inequality defining the boundaries for a parameter, eg: 0 <
 * paramName < 1, paramName > 0, etc.
 * @param isDefault :: Is this initialization a default one?
 * @return A pointer to the created Constraint
 */
IConstraint *ConstraintFactoryImpl::createInitialized(IFunction *fun,
                                                      const std::string &input,
                                                      bool isDefault) const {
  Expression expr;
  expr.parse(input);
  return createInitialized(fun, expr, isDefault);
}

/** An overloaded method using Expression.
 * @param fun :: The function
 * @param expr :: A parsed initialization Expression.
 * @param isDefault :: Is this initialization a default one?
 * @return A pointer to the created Constraint
 */
IConstraint *ConstraintFactoryImpl::createInitialized(IFunction *fun,
                                                      const Expression &expr,
                                                      bool isDefault) const {
  IConstraint *c = 0;
  if (expr.name() == "==") {
    c = createUnwrapped("BoundaryConstraint");
  } else {
    c = createUnwrapped(expr.name());
  }
  c->initialize(fun, expr, isDefault);
  return c;
}

} // namespace API
} // namespace Mantid
