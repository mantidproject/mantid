//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

SimplexMinimizer::SimplexMinimizer( gsl_multimin_function& gslContainer, 
  gsl_vector* startGuess, const double& size) : m_name("Simplex") 
{
  const gsl_multimin_fminimizer_type *simplexType = gsl_multimin_fminimizer_nmsimplex;

  // step size for simplex
  m_simplexStepSize = gsl_vector_alloc(gslContainer.n);
  gsl_vector_set_all (m_simplexStepSize, size);  // is this always a sensible starting step size?

  // setup minimizer
  m_gslSolver = gsl_multimin_fminimizer_alloc(simplexType, gslContainer.n);
  gsl_multimin_fminimizer_set(m_gslSolver, &gslContainer, startGuess, m_simplexStepSize);
}

SimplexMinimizer::~SimplexMinimizer()
{
  gsl_vector_free(m_simplexStepSize);
  gsl_multimin_fminimizer_free(m_gslSolver);
}

std::string SimplexMinimizer::name()const
{
  return m_name;
}

int SimplexMinimizer::iterate() 
{
  return gsl_multimin_fminimizer_iterate(m_gslSolver);
}

int SimplexMinimizer::hasConverged()
{
  double size = gsl_multimin_fminimizer_size(m_gslSolver);
  return gsl_multimin_test_size(size, 1e-2);
}

double SimplexMinimizer::costFunctionVal()
{
  return m_gslSolver->fval;
}

} // namespace CurveFitting
} // namespace Mantid
