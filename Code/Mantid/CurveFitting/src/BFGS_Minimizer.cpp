//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BFGS_Minimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

BFGS_Minimizer::BFGS_Minimizer( 
  gsl_multimin_function_fdf& gslContainer, 
  gsl_vector* startGuess,
  gsl_multifit_function_fdf& gslLeastSquaresContainer) 
  : m_name("BFGS") 
{
  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_vector_bfgs2;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, gslContainer.n);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &gslContainer, startGuess, 0.01, 0.01);

  m_gslLeastSquaresContainer = &gslLeastSquaresContainer;
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
  gsl_matrix * holdCalculatedJacobian;
  holdCalculatedJacobian =  gsl_matrix_alloc (m_gslLeastSquaresContainer->n, m_gslLeastSquaresContainer->p);

  int dummy = m_gslLeastSquaresContainer->df(m_gslSolver->x, m_gslLeastSquaresContainer->params, holdCalculatedJacobian);
  gsl_multifit_covar (holdCalculatedJacobian, epsrel, covar);

  gsl_matrix_free (holdCalculatedJacobian);
}

} // namespace CurveFitting
} // namespace Mantid
