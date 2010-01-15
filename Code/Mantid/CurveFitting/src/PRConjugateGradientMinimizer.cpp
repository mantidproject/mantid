//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/PRConjugateGradientMinimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

PRConjugateGradientMinimizer::PRConjugateGradientMinimizer( gsl_multimin_function_fdf& gslContainer, 
  gsl_vector* startGuess) : m_name("Polak-Ribiere conjugate gradient") 
{
  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_conjugate_pr;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, gslContainer.n);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &gslContainer, startGuess, 0.01, 1e-4);
}

PRConjugateGradientMinimizer::~PRConjugateGradientMinimizer()
{
  gsl_multimin_fdfminimizer_free(m_gslSolver);
}

std::string PRConjugateGradientMinimizer::name()const
{
  return m_name;
}

int PRConjugateGradientMinimizer::iterate() 
{
  return gsl_multimin_fdfminimizer_iterate(m_gslSolver);
}

int PRConjugateGradientMinimizer::hasConverged()
{
  return gsl_multimin_test_gradient(m_gslSolver->gradient, 1e-3);
}

double PRConjugateGradientMinimizer::costFunctionVal()
{
  return m_gslSolver->f;
}

void PRConjugateGradientMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  Kernel::Exception::NotImplementedError("Covariance matrix calculation for Simplex not implemented.");
}

} // namespace CurveFitting
} // namespace Mantid
