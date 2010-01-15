//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FRConjugateGradientMinimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

FRConjugateGradientMinimizer::FRConjugateGradientMinimizer( gsl_multimin_function_fdf& gslContainer, 
  gsl_vector* startGuess) : m_name("Fletcher-Reeves conjugate gradient") 
{
  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_conjugate_fr;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, gslContainer.n);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &gslContainer, startGuess, 0.01, 1e-4);
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
  Kernel::Exception::NotImplementedError("Covariance matrix calculation for Simplex not implemented.");
}

} // namespace CurveFitting
} // namespace Mantid
