//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/AdditionFunction.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

DECLARE_FUNCTION(AdditionFunction)

/** Ascertain if a function is of the same class. If so, its
 * component functions will be incorporated separately when combined
 * with this object.
 *  @param f :: pointer to the query function
 */
bool AdditionFunction::isAssociative(API::IFunction_sptr f) const {
  if (boost::dynamic_pointer_cast<AdditionFunction>(f)) {
    return true;
  }
  return false;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid