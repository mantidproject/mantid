/*WIKI*
A ProductFunction is an extension of the [[CompositeFunction]] which
multiplies its member functions to produce the output. Use this function
to construct a product of two or more fitting functions defined in
Mantid. A member of a ProductFunction can be a composite function itself.
 *WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ProductFunction.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid
{
namespace CurveFitting
{

DECLARE_FUNCTION(ProductFunction)

/** Function you want to fit to. 
 *  @param domain :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
 */
void ProductFunction::function(const API::FunctionDomain& domain, API::FunctionValues& values)const
{
  API::FunctionValues tmp(domain);
  values.setCalculated(1.0);
  for(size_t iFun = 0; iFun < nFunctions(); ++iFun)
  {
    domain.reset();
    getFunction( iFun )->function(domain,tmp);
    values *= tmp;
  }
}

/**
 * Calculate the derivatives numerically.
 * @param domain :: Function domein.
 * @param jacobian :: Jacobian - stores the calculated derivatives
 */
void ProductFunction::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  calNumericalDeriv(domain,jacobian);
}


} // namespace CurveFitting
} // namespace Mantid
