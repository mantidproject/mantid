//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/PRConjugateGradientMinimizer.h"
#include "MantidCurveFitting/CostFunctionFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{
///@cond nodoc
DECLARE_FUNCMINIMIZER(PRConjugateGradientMinimizer,Conjugate gradient (Polak-Ribiere imp.))
///@endcond

// Get a reference to the logger
Kernel::Logger& PRConjugateGradientMinimizer::g_log = Kernel::Logger::get("PRConjugateGradientMinimizer");


void PRConjugateGradientMinimizer::initialize(double* X, const double* Y, 
                                              double *sqrtWeight, const int& nData, 
                                              const int& nParam, gsl_vector* startGuess, API::IFitFunction* function, 
                                              const std::string& costFunction) 
{
  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));

  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_conjugate_pr;

  // setup GSL container
  m_gslMultiminContainer.n = nParam;  
  m_gslMultiminContainer.f = &gsl_costFunction;
  m_gslMultiminContainer.df = &gsl_costFunction_df;
  m_gslMultiminContainer.fdf = &gsl_costFunction_fdf;
  m_gslMultiminContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, nParam);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &m_gslMultiminContainer, startGuess, 0.01, 1e-4);

  // for covariance matrix
  m_gslLeastSquaresContainer.f = &gsl_f;
  m_gslLeastSquaresContainer.df = &gsl_df;
  m_gslLeastSquaresContainer.fdf = &gsl_fdf;
  m_gslLeastSquaresContainer.n = nData;
  m_gslLeastSquaresContainer.p = nParam;
  m_gslLeastSquaresContainer.params = m_data;
}

void PRConjugateGradientMinimizer::initialize(API::IFitFunction* function, const std::string& costFunction) 
{
  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));

  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_conjugate_pr;

  // setup GSL container
  m_gslMultiminContainer.n = m_data->n;  
  m_gslMultiminContainer.f = &gsl_costFunction;
  m_gslMultiminContainer.df = &gsl_costFunction_df;
  m_gslMultiminContainer.fdf = &gsl_costFunction_fdf;
  m_gslMultiminContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, m_data->p);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &m_gslMultiminContainer, m_data->initFuncParams, 0.01, 1e-4);

  // for covariance matrix
  m_gslLeastSquaresContainer.f = &gsl_f;
  m_gslLeastSquaresContainer.df = &gsl_df;
  m_gslLeastSquaresContainer.fdf = &gsl_fdf;
  m_gslLeastSquaresContainer.n = m_data->n;
  m_gslLeastSquaresContainer.p = m_data->p;
  m_gslLeastSquaresContainer.params = m_data;
}

PRConjugateGradientMinimizer::~PRConjugateGradientMinimizer()
{
  delete m_data;
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

/* Calculates covariance matrix
 *
 * @param epsrel Is used to remove linear-dependent columns
 * @param covar Returned covariance matrix, here as 
 */
void PRConjugateGradientMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  gsl_matrix * holdCalculatedJacobian;
  holdCalculatedJacobian =  gsl_matrix_alloc (m_gslLeastSquaresContainer.n, m_gslLeastSquaresContainer.p);

  int dummy = m_gslLeastSquaresContainer.df(m_gslSolver->x, m_gslLeastSquaresContainer.params, holdCalculatedJacobian);
  (void) dummy;
  gsl_multifit_covar (holdCalculatedJacobian, epsrel, covar);

  gsl_matrix_free (holdCalculatedJacobian);
}

} // namespace CurveFitting
} // namespace Mantid
