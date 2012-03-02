//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMDMinimizer.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"

//#include "MantidAPI/FunctionDomain1D.h"
//#include "MantidAPI/FunctionValues.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <iostream>
//#include <fstream>

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCMINIMIZER(LevenbergMarquardtMDMinimizer,Levenberg-MarquardtMD)

// Get a reference to the logger
Kernel::Logger& LevenbergMarquardtMDMinimizer::g_log = Kernel::Logger::get("LevenbergMarquardtMDMinimizer");

/// Constructor
LevenbergMarquardtMDMinimizer::LevenbergMarquardtMDMinimizer():
IFuncMinimizer(),
m_relTol(1e-6),
m_tau(1e-6),
m_mu(0),
m_nu(2.0),
m_rho(1.0)
{
}

/// Initialize minimizer, i.e. pass a function to minimize.
void LevenbergMarquardtMDMinimizer::initialize(API::ICostFunction_sptr function)
{
  m_leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(function);
  if ( !m_leastSquares )
  {
    throw std::invalid_argument("Levenberg-Marquardt minimizer works only with least squares. Different function was given.");
  }
  m_mu = 0;
  m_nu = 2.0;
  m_rho = 1.0;
  m_oldDder = 0.0;
}

/// Do one iteration.
bool LevenbergMarquardtMDMinimizer::iterate()
{
  bool good = false;
  const bool debug = false;
  if ( !m_leastSquares )
  {
    throw std::runtime_error("Cost function isn't set up.");
  }
  size_t n = m_leastSquares->nParams();

  if (m_der.size() == 0)
  {
    m_der.resize(n);
    m_hessian.resize(n,n);
  }
  // calculate the first and second derivatives of the cost function.
  if (m_mu == 0.0)
  {// first time calculate everything
    m_F = m_leastSquares->valDerivHessian(m_der, m_hessian);
  }
  else if (m_rho > 0)
  {// last iteration was good: calculate new m_der and m_hessian, dont't recalculate m_F
    m_leastSquares->valDerivHessian(m_der, m_hessian, false);
    good = true;
  }
  // else if m_rho < 0 last iteration was bad: reuse m_der and m_hessian

  // Calculate damping to hessian
  if (m_mu == 0) // first iteration
  {
    m_mu = m_tau;// * maxH;
    m_nu = 2.0;
  }

  if (debug)
  {
    std::cerr << "mu=" << m_mu << std::endl;
  }

  if (m_D.empty())
  {
    m_D.resize(n);
  }

  // copy the hessian
  GSLMatrix H(m_hessian);
  for(size_t i = 0; i < n; ++i)
  {
    double d = fabs(m_der.get(i));
    if (m_D[i] > d) d = m_D[i];
    m_D[i] = d;
    double tmp = H.get(i,i) + m_mu * d;
    H.set(i,i,tmp);
  }

  if (debug && m_rho > 0)
  {
    std::cerr << "H:" << std::endl;
    for(size_t i = 0; i < n; ++i)
    {
      for(size_t j = 0; j < n; ++j)
      {
        std::cerr << H.get(i,j) << ' ';
      }
      std::cerr << std::endl;
    }
    std::cerr << "-----------------------------\n";
  }

  /// Parameter corrections
  GSLVector dx(n);
  GSLVector dd(m_der);
  // To find dx solve the system of linear equations   H * dx == -m_der
  int s;
  // multiply the derivatives by -1
  gsl_blas_dscal(-1.0,dd.gsl());
  gsl_permutation * p = gsl_permutation_alloc( n );
  gsl_linalg_LU_decomp( H.gsl(), p, &s ); // H is modified at this moment
  gsl_linalg_LU_solve( H.gsl(), p, dd.gsl(), dx.gsl() );
  gsl_permutation_free( p );

  if (debug)
  {
    for(size_t j = 0; j < n; ++j)  {std::cerr << m_der.get(j) << ' '; } std::cerr << std::endl;
    for(size_t j = 0; j < n; ++j)  {std::cerr << dx.get(j) << ' '; } std::cerr << std::endl << std::endl;
    //system("pause");
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
  
  double dder = sqrt( gsl_blas_dnrm2( m_der.gsl() )  );
  if (debug)
  {
    std::cerr << "dder=" << dder << std::endl;
  }

  // --- prepare for the next iteration --- //

  double dL;
  // der -> - der - 0.5 * hessian * dx 
  gsl_blas_dgemv( CblasNoTrans,-0.5, m_hessian.gsl(), dx.gsl(), 1., dd.gsl() );
  // calculate the linear part of the change in cost function
  // dL = - der * dx - 0.5 * dx * hessian * dx
  gsl_blas_ddot( dd.gsl(), dx.gsl(), &dL );

  double F1 = m_leastSquares->val();
  if (debug)
  {
    static size_t iter = 0;
    std::cerr << "iter " << ++iter << std::endl;
    std::cerr << "F " << m_F << ' ' << F1 << ' ' << dL << std::endl;
  }

  // Try the stop condition
  //if (fabs(dL) < m_relTol)
  //if (m_oldDder > 0.0 && dder < 0.001)
  if (good)
  {
    GSLVector p(n);
    m_leastSquares->getParameters(p);
    double dx_norm = gsl_blas_dnrm2(dx.gsl());
    double p_norm = gsl_blas_dnrm2(p.gsl());
    if (dx_norm < 0.0001)
    {
      if (debug)
      std::cerr << "stopped at " << dder << ' ' << fabs(m_oldDder - dder)/m_oldDder << std::endl;
      return false;
    }
  }
  m_oldDder = dder;

  if (fabs(dL) == 0.0) m_rho = 0;
  else
    m_rho = (m_F - F1) / dL;
  if (debug)
  {
    std::cerr << "rho=" << m_rho << std::endl;
  }

  if (m_rho > 0)
  {// good progress, decrease m_mu but no more than by 1/3
    // rho = 1 - (2*rho - 1)^3
    m_rho = 2.0 * m_rho - 1.0;
    m_rho = 1.0 - m_rho * m_rho * m_rho;
    const double I3 = 1.0 / 3.0;
    if (m_rho > I3) m_rho = I3;
    if (m_rho < 0.0001) m_rho = 0.1;
    m_mu *= m_rho;
    //if (m_mu < m_tau) m_mu = m_tau;
    m_nu = 2.0;
    m_F = F1;
    if (debug)
    std::cerr << "times " << m_rho << std::endl;
  }
  else
  {// bad iteration. increase m_mu and revert changes to parameters
    m_mu *= m_nu;
    m_nu *= 2.0;
    // undo parameter update
    for(size_t i = 0; i < n; ++i)
    {
      double d = m_leastSquares->getParameter(i) - dx.get(i);
      m_leastSquares->setParameter(i,d);
      //std::cerr << "P" << i << ' ' << d << std::endl;
    }
    m_leastSquares->getFittingFunction()->applyTies();
    m_F = m_leastSquares->val();
  }

  return true;
}

/// Return current value of the cost function
double LevenbergMarquardtMDMinimizer::costFunctionVal()
{
  if ( !m_leastSquares )
  {
    throw std::runtime_error("Cost function isn't set up.");
  }
  return m_leastSquares->val();
}

} // namespace CurveFitting
} // namespace Mantid
