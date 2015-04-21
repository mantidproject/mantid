#include "MantidAPI/ILatticeFunction.h"

namespace Mantid {
namespace API {

using namespace Geometry;

ILatticeFunction::ILatticeFunction() : FunctionParameterDecorator() {}

/**
 * Implementation of IFunction::function
 *
 * The implementation tries to cast the supplied domain to LatticeDomain,
 * and calls functionLattice, which needs to be implemented in subclasses.
 *
 * @param domain :: A FunctionDomain of type LatticeDomain
 * @param values :: Function values.
 */
void ILatticeFunction::function(const FunctionDomain &domain,
                                FunctionValues &values) const {
  try {
    const LatticeDomain &latticeDomain =
        dynamic_cast<const LatticeDomain &>(domain);

    functionLattice(latticeDomain, values);
  }
  catch (std::bad_cast) {
    throw std::invalid_argument(
        "ILatticeFunction expects domain of type LatticeDomain.");
  }
}

/**
 * Implementation of IFunction::functionDeriv
 *
 * Just like the function-method, the domain is checked for correct type. If
 * functionDerivLattice has not been implemented, numerical derivatives are
 * calculated.
 *
 * @param domain :: A FunctionDomain of type LatticeDomain
 * @param jacobian :: Jacobian matrix
 */
void ILatticeFunction::functionDeriv(const FunctionDomain &domain,
                                     Jacobian &jacobian) {
  try {
    const LatticeDomain &latticeDomain =
        dynamic_cast<const LatticeDomain &>(domain);

    functionDerivLattice(latticeDomain, jacobian);
  }
  catch (std::bad_cast) {
    throw std::invalid_argument(
        "ILatticeFunction expects domain of type LatticeDomain.");
  }
  catch (Kernel::Exception::NotImplementedError) {
    calNumericalDeriv(domain, jacobian);
  }
}

/// Default implementation, throws NotImplementedError.
void ILatticeFunction::functionDerivLattice(const LatticeDomain &latticeDomain,
                                            Jacobian &jacobian) {
  UNUSED_ARG(latticeDomain);
  UNUSED_ARG(jacobian);

  throw Kernel::Exception::NotImplementedError(
      "FunctionLatticeDeriv is not implemented for this function.");
}

} // namespace API
} // namespace Mantid
