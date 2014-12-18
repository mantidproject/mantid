//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DampingMinimizer.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFunction.h"

#include "MantidKernel/Logger.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_blas.h>
#include <iostream>
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{
  namespace
  {
    /// static logger
    Kernel::Logger g_log("DampingMinimizer");
  }

DECLARE_FUNCMINIMIZER(DampingMinimizer,Damping)


/// Constructor
DampingMinimizer::DampingMinimizer():
IFuncMinimizer(),
m_relTol(1e-6)
{
  declareProperty("Damping",0.0,"The damping parameter.");
}

/// Initialize minimizer, i.e. pass a function to minimize.
void DampingMinimizer::initialize(API::ICostFunction_sptr function,size_t)
{
  m_leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(function);
  if ( !m_leastSquares )
  {
    throw std::invalid_argument("Damping minimizer works only with least squares. Different function was given.");
  }
}

/// Do one iteration.
bool DampingMinimizer::iterate(size_t)
{
  const bool debug = false;

  const double damping = getProperty("Damping");

  if ( !m_leastSquares )
  {
    throw std::runtime_error("Cost function isn't set up.");
  }
  size_t n = m_leastSquares->nParams();

  if ( n == 0 )
  {
    m_errorString = "No parameters to fit";
    return false;
  }

  // calculate the first and second derivatives of the cost function.
  m_leastSquares->valDerivHessian();

  // copy the hessian
  GSLMatrix H( m_leastSquares->getHessian() );
  GSLVector dd( m_leastSquares->getDeriv() );

  for(size_t i = 0; i < n; ++i)
  {
    double tmp = H.get(i,i) + damping;
    H.set(i,i,tmp);
  }

  if ( debug )
  {
    std::cerr << "H:\n" << H ;
    std::cerr << "-----------------------------\n";
    for(size_t j = 0; j < n; ++j)  {std::cerr << dd.get(j) << ' '; } std::cerr << std::endl;
  }

  /// Parameter corrections
  GSLVector dx(n);
  // To find dx solve the system of linear equations   H * dx == -m_der
  dd *= -1.0;
  H.solve( dd, dx );

  if (debug)
  {
    for(size_t j = 0; j < n; ++j)  {std::cerr << dx.get(j) << ' '; } std::cerr << std::endl << std::endl;
  }

  // Update the parameters of the cost function.
  for(size_t i = 0; i < n; ++i)
  {
    double d = m_leastSquares->getParameter(i) + dx.get(i);
    m_leastSquares->setParameter(i,d);
    if (debug)
    {
      std::cerr << "P" << i << ' ' << d << std::endl;
    }
  }
  m_leastSquares->getFittingFunction()->applyTies();
  
  // --- prepare for the next iteration --- //

  // Try the stop condition
  GSLVector p(n);
  m_leastSquares->getParameters(p);
  double dx_norm = gsl_blas_dnrm2(dx.gsl());
  //double p_norm = gsl_blas_dnrm2(p.gsl());
  if (dx_norm < 0.0001)
  {
    return false;
  }

  return true;
}

/// Return current value of the cost function
double DampingMinimizer::costFunctionVal()
{
  if ( !m_leastSquares )
  {
    throw std::runtime_error("Cost function isn't set up.");
  }
  return m_leastSquares->val();
}

} // namespace CurveFitting
} // namespace Mantid
