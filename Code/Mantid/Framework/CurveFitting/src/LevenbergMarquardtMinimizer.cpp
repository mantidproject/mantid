//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <iostream>

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCMINIMIZER(LevenbergMarquardtMinimizer,Levenberg-Marquardt)

// Get a reference to the logger
Kernel::Logger& LevenbergMarquardtMinimizer::g_log = Kernel::Logger::get("LevenbergMarquardtMinimizer");

/// Initialize minimizer, i.e. pass a function to minimize.
void LevenbergMarquardtMinimizer::initialize(API::ICostFunction_sptr function)
{
  m_leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(function);
  if ( !m_leastSquares )
  {
    throw std::invalid_argument("Levenberg-Marquardt minimizer works only with least squares. Different function was given.");
  }
}

/// Do one iteration.
bool LevenbergMarquardtMinimizer::iterate()
{
  if ( !m_leastSquares )
  {
    throw std::runtime_error("Cost function isn't set up.");
  }
  size_t n = m_leastSquares->nParams();
  GSLVector der(n);
  GSLMatrix hessian(n,n);
  m_leastSquares->valDerivHessian(der, hessian);

  // multiply the derivatives by -1
  gsl_blas_dscal(-1.0,der.gsl());

  // Parameter corrections
  GSLVector dx(n);

  int s;
  gsl_permutation * p = gsl_permutation_alloc( n );

  gsl_linalg_LU_decomp( hessian.gsl(), p, &s );
  gsl_linalg_LU_solve( hessian.gsl(), p, der.gsl(), dx.gsl() );

  gsl_permutation_free( p );

  GSLVector param(n);
  for(size_t i = 0; i < n; ++i)
  {
    double d = m_leastSquares->getParameter(i) + dx.get(i);
    m_leastSquares->setParameter(i,d);
    param.set(i,d);
  }
  
  double paramNorm = gsl_blas_dnrm2( param.gsl() );
  double dxNorm = gsl_blas_dnrm2( dx.gsl() );

  if (dxNorm <= 0.0001 * paramNorm )
  {
    return false;
  }

  return true;
}

/// Return current value of the cost function
double LevenbergMarquardtMinimizer::costFunctionVal()
{
  if ( !m_leastSquares )
  {
    throw std::runtime_error("Cost function isn't set up.");
  }
  return m_leastSquares->val();
}

} // namespace CurveFitting
} // namespace Mantid
