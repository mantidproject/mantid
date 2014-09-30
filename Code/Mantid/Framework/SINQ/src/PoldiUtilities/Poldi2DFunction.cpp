#include "MantidSINQ/PoldiUtilities/Poldi2DFunction.h"

namespace Mantid
{
namespace Poldi
{
using namespace API;

Poldi2DFunction::Poldi2DFunction() :
    IFunction1DSpectrum(),
    CompositeFunction()
{
}

/**
 * Calculates function values for domain. In contrast to CompositeFunction, the summation
 * of values is performed in a different way: The values are set to zero once at the beginning,
 * then it is expected of the member functions to use FunctionValues::addToCalculated or add
 * their values otherwise, without erasing the values.
 *
 * @param domain :: Function domain which is passed on to the member functions.
 * @param values :: Function values.
 */
void Poldi2DFunction::function(const FunctionDomain &domain, FunctionValues &values) const
{
    CompositeFunction::function(domain, values);
}

/**
 * Calculates function derivatives. Simply calls CompositeFunction::functionDeriv
 *
 * @param domain :: Function domain which is passed on to the member functions.
 * @param jacobian :: Jacobian.
 */
void Poldi2DFunction::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian)
{
    CompositeFunction::functionDeriv(domain, jacobian);
}

/**
 * Empty implementation of IFunction1DSpectrum::function1DSpectrum which must be present to
 * completely implement the interface.
 *
 * @param domain :: Unused domain.
 * @param values :: Unused function values.
 */
void Poldi2DFunction::function1DSpectrum(const FunctionDomain1DSpectrum &domain, FunctionValues &values) const
{
    UNUSED_ARG(domain);
    UNUSED_ARG(values);
}



} // namespace Poldi
} // namespace Mantid
