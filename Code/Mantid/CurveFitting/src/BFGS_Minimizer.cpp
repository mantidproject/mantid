//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BFGS_Minimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

BFGS_Minimizer::BFGS_Minimizer( gsl_multimin_function_fdf& gslContainer, 
  gsl_vector* startGuess) : m_name("BFGS") 
{
  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_vector_bfgs2;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, gslContainer.n);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &gslContainer, startGuess, 0.01, 0.01);
}

BFGS_Minimizer::~BFGS_Minimizer()
{
  gsl_multimin_fdfminimizer_free(m_gslSolver);
}

std::string BFGS_Minimizer::name()const
{
  return m_name;
}

int BFGS_Minimizer::iterate() 
{
  return gsl_multimin_fdfminimizer_iterate(m_gslSolver);
}

int BFGS_Minimizer::hasConverged()
{
  return gsl_multimin_test_gradient(m_gslSolver->gradient, 1e-3);
}

double BFGS_Minimizer::costFunctionVal()
{
  return m_gslSolver->f;
}

void BFGS_Minimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  Kernel::Exception::NotImplementedError("Covariance matrix calculation for Simplex not implemented.");
}

} // namespace CurveFitting
} // namespace Mantid
