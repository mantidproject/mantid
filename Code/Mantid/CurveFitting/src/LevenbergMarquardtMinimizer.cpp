//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include <gsl/gsl_blas.h>
#include "MantidKernel/Exception.h"
#include <iostream>

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

  m_gslContainer = &gslContainer;
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
  int retVal = gsl_multifit_fdfsolver_iterate(m_gslSolver);

  // From experience it is found that gsl_multifit_fdfsolver_iterate occasionally get
  // stock - even after having achieved a sensible fit. This seem in particular to be a
  // problem on Linux. However, to force GSL not to return ga ga have to do stuff in the
  // if statement below
  /*if (retVal == GSL_CONTINUE)
  {
    gsl_vector * dummy;
    dummy = gsl_vector_alloc (m_gslContainer->n);
    std::cout << "\nboevs " <<  m_gslContainer->n << std::endl;
    try
    {
      double something = m_gslContainer->f(m_gslSolver->x, m_gslContainer, dummy);
    }
    catch(...)
    {

    }
    gsl_vector_free (dummy);
  }*/

  return retVal;
}

int LevenbergMarquardtMinimizer::hasConverged()
{
  return gsl_multifit_test_delta(m_gslSolver->dx, m_gslSolver->x, 1e-4, 1e-4);
}

double LevenbergMarquardtMinimizer::costFunctionVal()
{
  double chi = gsl_blas_dnrm2(m_gslSolver->f);
  return chi*chi;
}

void LevenbergMarquardtMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  gsl_multifit_covar (m_gslSolver->J, epsrel, covar);
}

} // namespace CurveFitting
} // namespace Mantid
