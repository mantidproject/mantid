//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FRConjugateGradientMinimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

FRConjugateGradientMinimizer::FRConjugateGradientMinimizer( 
  gsl_multimin_function_fdf& gslContainer, 
  gsl_vector* startGuess, 
  gsl_multifit_function_fdf& gslLeastSquaresContainer ) 
  : m_name("Fletcher-Reeves conjugate gradient") 
{
  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_conjugate_fr;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, gslContainer.n);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &gslContainer, startGuess, 0.01, 1e-4);

  m_gslLeastSquaresContainer = &gslLeastSquaresContainer;
}

FRConjugateGradientMinimizer::~FRConjugateGradientMinimizer()
{
  gsl_multimin_fdfminimizer_free(m_gslSolver);
}

std::string FRConjugateGradientMinimizer::name()const
{
  return m_name;
}

int FRConjugateGradientMinimizer::iterate() 
{
  return gsl_multimin_fdfminimizer_iterate(m_gslSolver);
}

int FRConjugateGradientMinimizer::hasConverged()
{
  return gsl_multimin_test_gradient(m_gslSolver->gradient, 1e-3);
}

double FRConjugateGradientMinimizer::costFunctionVal()
{
  return m_gslSolver->f;
}

void FRConjugateGradientMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  gsl_matrix * holdCalculatedJacobian;
  holdCalculatedJacobian =  gsl_matrix_alloc (m_gslLeastSquaresContainer->n, m_gslLeastSquaresContainer->p);

  int dummy = m_gslLeastSquaresContainer->df(m_gslSolver->x, m_gslLeastSquaresContainer->params, holdCalculatedJacobian);
  gsl_multifit_covar (holdCalculatedJacobian, epsrel, covar);

  gsl_matrix_free (holdCalculatedJacobian);
}

} // namespace CurveFitting
} // namespace Mantid
