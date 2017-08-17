#include "MantidAPI/AssociativeCompositeFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {

namespace {
/// static logger
Kernel::Logger g_log("AssociativeCompositeFunction");
}

/** We do not declare this function, since it is pure virtual
 * DECLARE_FUNCTION(AssociativeCompositeFunction)
*/

/** Add a function to the end of the vector of component functions
 * @param f :: A pointer to the added function
 * @return The index of the last added function
 */
std::size_t AssociativeCompositeFunction::addFunction(IFunction_sptr f) {
    if (isAssociative(f)) {
        auto fa = boost::dynamic_pointer_cast<AssociativeCompositeFunction>(f);
        for(std::size_t i=0; i < fa->nFunctions(); i++) {
            CompositeFunction::addFunction(fa->getFunction(i));
        }
    } else {
        CompositeFunction::addFunction(f);
    }
    return nFunctions() - 1;
}

/** Insert a function at a given index in the vector of component functions
 *  @param i :: The index assigned to the new function.
 *  @param f :: A pointer to the new function
 */
void AssociativeCompositeFunction::insertFunction(size_t i, IFunction_sptr f) {
    if (isAssociative(f)) {
        auto fa = boost::dynamic_pointer_cast<AssociativeCompositeFunction>(f);
        for(std::size_t i=0; i < fa->nFunctions(); i++) {
            CompositeFunction::insertFunction(i, fa->getFunction(i));
        }
    } else {
        CompositeFunction::insertFunction(i, f);
    }
}

/** Replace a function with a new one. The old function is deleted.
 * @param i :: The index of the function to replace
 * @param f :: A pointer to the new function
 */
void AssociativeCompositeFunction::replaceFunction(size_t i, IFunction_sptr f) {
    if (isAssociative(f)) {
        removeFunction(i);
        insertFunction(i, f);
    } else {
        CompositeFunction::replaceFunction(i, f);
    }
}

}
}