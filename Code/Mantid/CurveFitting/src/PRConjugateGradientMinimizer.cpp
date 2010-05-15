//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/PRConjugateGradientMinimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

PRConjugateGradientMinimizer::PRConjugateGradientMinimizer( 
  gsl_multimin_function_fdf& gslContainer, 
  gsl_vector* startGuess,
  gsl_multifit_function_fdf& gslLeastSquaresContainer ) 
  : m_name("Polak-Ribiere conjugate gradient") 
{
  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_conjugate_pr;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, gslContainer.n);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &gslContainer, startGuess, 0.01, 1e-4);

  m_gslLeastSquaresContainer = &gslLeastSquaresContainer;
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
  gsl_matrix * holdCalculatedJacobian;
  holdCalculatedJacobian =  gsl_matrix_alloc (m_gslLeastSquaresContainer->n, m_gslLeastSquaresContainer->p);

  int dummy = m_gslLeastSquaresContainer->df(m_gslSolver->x, m_gslLeastSquaresContainer->params, holdCalculatedJacobian);
  gsl_multifit_covar (holdCalculatedJacobian, epsrel, covar);

  gsl_matrix_free (holdCalculatedJacobian);
}

} // namespace CurveFitting
} // namespace Mantid
