//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_blas.h>
#include <iostream>

namespace Mantid
{
namespace CurveFitting
{
  namespace
  {
    // Get a reference to the logger
    Kernel::Logger g_log("LevenbergMarquardtMinimizer");
  }

DECLARE_FUNCMINIMIZER(LevenbergMarquardtMinimizer,Levenberg-Marquardt)


LevenbergMarquardtMinimizer::LevenbergMarquardtMinimizer() 
  : m_data(NULL), gslContainer(), m_gslSolver(NULL),m_function(), m_absError(1e-4), m_relError(1e-4)
{
  declareProperty("AbsError", m_absError, "Absolute error allowed for parameters - a stopping parameter in success.");
  declareProperty("RelError", m_relError, "Relative error allowed for parameters - a stopping parameter in success.");
}

void LevenbergMarquardtMinimizer::initialize(API::ICostFunction_sptr costFunction, size_t)
{
  // set-up GSL container to be used with GSL simplex algorithm
  auto leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(costFunction);
  if ( leastSquares )
  {
    m_data = new GSL_FitData( leastSquares );
  }
  else
  {
    throw std::runtime_error("LevenbergMarquardt can only be used with Least squares cost function.");
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
  if (!m_gslSolver)
  {
    throw std::runtime_error("Levenberg-Marquardt minimizer failed to initialize. \n"+
      boost::lexical_cast<std::string>(m_data->n)+" data points, "+
      boost::lexical_cast<std::string>(m_data->p)+" fitting parameters. "
      );
  }
  gsl_multifit_fdfsolver_set(m_gslSolver, &gslContainer, m_data->initFuncParams);

  m_function = leastSquares->getFittingFunction();

}

LevenbergMarquardtMinimizer::~LevenbergMarquardtMinimizer()
{
  if ( m_data )
  {
    delete m_data;
  }
  if ( m_gslSolver )
  {
    gsl_multifit_fdfsolver_free(m_gslSolver);
  }
}

bool LevenbergMarquardtMinimizer::iterate(size_t)
{
  m_absError = getProperty("AbsError");
  m_relError = getProperty("RelError");

  int retVal = gsl_multifit_fdfsolver_iterate(m_gslSolver);

  // From experience it is found that gsl_multifit_fdfsolver_iterate occasionally get
  // stock - even after having achieved a sensible fit. This seem in particular to be a
  // problem on Linux. However, to force GSL not to return ga ga have to do stuff in the
  // if statement below
  // GSL 1.14 changed return value from GSL_CONTINUE->GSL_ENOPROG for non-converging fits at 10 iterations
  if( retVal == GSL_CONTINUE || retVal == GSL_ENOPROG ) 
  {
    size_t ia = 0;
    for (size_t i = 0; i < m_function->nParams(); i++)
    {
      if (m_function->isActive(i))
      {
        m_function->setActiveParameter(i,gsl_vector_get(m_gslSolver->x,ia));
        ++ia;
      }
    }
    m_function->applyTies();
    retVal = GSL_CONTINUE;
  }

  if(retVal && retVal != GSL_CONTINUE)
  {
    m_errorString = gsl_strerror(retVal);
    return false;
  }

  retVal = hasConverged();
  m_errorString = gsl_strerror(retVal);

  return retVal != GSL_SUCCESS;
}

int LevenbergMarquardtMinimizer::hasConverged()
{
  return gsl_multifit_test_delta(m_gslSolver->dx, m_gslSolver->x, m_absError, m_relError);
}

double LevenbergMarquardtMinimizer::costFunctionVal()
{
  double chi = gsl_blas_dnrm2(m_gslSolver->f);
  return chi*chi;
}

/* Calculates covariance matrix
 *
 * @param epsrel :: Is used to remove linear-dependent columns
 * @param covar :: Returned covariance matrix, here as 
 */
void LevenbergMarquardtMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  gsl_multifit_covar (m_gslSolver->J, epsrel, covar);
}

} // namespace CurveFitting
} // namespace Mantid
