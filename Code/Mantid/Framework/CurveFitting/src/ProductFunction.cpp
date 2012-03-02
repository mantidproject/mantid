//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ProductFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/DeltaFunction.h"
#include <cmath>
#include <algorithm>
#include <functional>
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

#include <sstream>

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
 * Derivatives of function with respect to active parameters
 * @param domain :: Function domain to get the arguments from.
 * @param jacobian :: A Jacobian to store the derivatives.
 */
void ProductFunction::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  this->calNumericalDeriv(domain,jacobian);
}

} // namespace CurveFitting
} // namespace Mantid
