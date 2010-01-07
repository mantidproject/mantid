//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_blas.h>
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

LevenbergMarquardtMinimizer::LevenbergMarquardtMinimizer(
  gsl_multifit_function_fdf& gslContainer, 
  gsl_vector* startGuess) : m_name("Levenberg Marquardt") 
{
  // specify the type of GSL solver to use
  const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;

  // setup GSL solver
  m_gslSolver = gsl_multifit_fdfsolver_alloc(T, gslContainer.n, gslContainer.p);
  gsl_multifit_fdfsolver_set(m_gslSolver, &gslContainer, startGuess);
}

LevenbergMarquardtMinimizer::~LevenbergMarquardtMinimizer()
{
  gsl_multifit_fdfsolver_free(m_gslSolver);
}

std::string LevenbergMarquardtMinimizer::name()const
{
  return m_name;
}

int LevenbergMarquardtMinimizer::iterate() 
{
  return gsl_multifit_fdfsolver_iterate(m_gslSolver);
}

int LevenbergMarquardtMinimizer::hasConverged()
{
  return gsl_multifit_test_delta(m_gslSolver->dx, m_gslSolver->x, 1e-4, 1e-4);
}

double LevenbergMarquardtMinimizer::costFunctionVal()
{
  return gsl_blas_dnrm2(m_gslSolver->f);
}

} // namespace CurveFitting
} // namespace Mantid
