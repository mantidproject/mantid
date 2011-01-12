//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include "MantidCurveFitting/CostFunctionFactory.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <gsl/gsl_blas.h>
#include <iostream>

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCMINIMIZER(LevenbergMarquardtMinimizer,Levenberg-Marquardt)

// Get a reference to the logger
Kernel::Logger& LevenbergMarquardtMinimizer::g_log = Kernel::Logger::get("LevenbergMarquardtMinimizer");

void LevenbergMarquardtMinimizer::initialize(double* X, const double* Y, 
                                             double *sqrtWeight, const int& nData, 
                                             const int& nParam, gsl_vector* startGuess, 
                                             API::IFitFunction* function, const std::string& costFunction)
{
  // set-up GSL container to be used with GSL simplex algorithm

  if ( costFunction.compare("Least squares") == 0 )
  {
    m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));
  }
  else
  {
    g_log.warning("LevenbergMarquardt can only be used with Least squares cost function. Default to Least squares\n");
    m_data = new GSL_FitData(function,new CostFuncLeastSquares());
  }

  // specify the type of GSL solver to use
  const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;

  // setup GSL container
  gslContainer.f = &gsl_f;
  gslContainer.df = &gsl_df;
  gslContainer.fdf = &gsl_fdf;
  gslContainer.n = nData;
  gslContainer.p = nParam;
  gslContainer.params = m_data;

  // setup GSL solver
  m_gslSolver = gsl_multifit_fdfsolver_alloc(T, nData, nParam);
  gsl_multifit_fdfsolver_set(m_gslSolver, &gslContainer, startGuess);

  m_function = function;

}

void LevenbergMarquardtMinimizer::initialize(API::IFitFunction* function, const std::string& costFunction)
{
  // set-up GSL container to be used with GSL simplex algorithm

  if ( costFunction.compare("Least squares") == 0 )
  {
    m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));
  }
  else
  {
    g_log.warning("LevenbergMarquardt can only be used with Least squares cost function. Default to Least squares\n");
    m_data = new GSL_FitData(function,new CostFuncLeastSquares());
  }

  // specify the type of GSL solver to use
  const gsl_multifit_fdfsolver_type *T = gsl_multifit_fdfsolver_lmsder;

  // setup GSL container
  gslContainer.f = &gsl_f;
  gslContainer.df = &gsl_df;
  gslContainer.fdf = &gsl_fdf;
  gslContainer.n = m_data->n;
  gslContainer.p = m_data->p;
  gslContainer.params = m_data;

  // setup GSL solver
  m_gslSolver = gsl_multifit_fdfsolver_alloc(T, m_data->n, m_data->p);
  gsl_multifit_fdfsolver_set(m_gslSolver, &gslContainer, m_data->initFuncParams);

  m_function = function;

}

LevenbergMarquardtMinimizer::~LevenbergMarquardtMinimizer()
{
  delete m_data;
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
  // GSL 1.14 changed return value from GSL_CONTINUE->GSL_ENOPROG for non-converging fits at 10 iterations
  if( retVal == GSL_CONTINUE || retVal == GSL_ENOPROG ) 
  {
    for (int i = 0; i < m_function->nActive(); i++)
    {
      m_function->setActiveParameter(i,gsl_vector_get(m_gslSolver->x,i));
    }
    retVal = GSL_CONTINUE;
  }

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

/* Calculates covariance matrix
 *
 * @param epsrel Is used to remove linear-dependent columns
 * @param covar Returned covariance matrix, here as 
 */
void LevenbergMarquardtMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  gsl_multifit_covar (m_gslSolver->J, epsrel, covar);
}

} // namespace CurveFitting
} // namespace Mantid
