//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BFGS_Minimizer.h"
#include "MantidCurveFitting/CostFunctionFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCMINIMIZER(BFGS_Minimizer,BFGS)

// Get a reference to the logger
Kernel::Logger& BFGS_Minimizer::g_log = Kernel::Logger::get("BFGS_Minimizer");

void BFGS_Minimizer::initialize(double* X, const double* Y, 
                                              double *sqrtWeight, const int& nData, 
                                              const int& nParam, gsl_vector* startGuess, API::IFitFunction* function, 
                                              const std::string& costFunction) 
{
  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));

  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_vector_bfgs2;

  // setup GSL container
  m_gslMultiminContainer.n = nParam;  
  m_gslMultiminContainer.f = &gsl_costFunction;
  m_gslMultiminContainer.df = &gsl_costFunction_df;
  m_gslMultiminContainer.fdf = &gsl_costFunction_fdf;
  m_gslMultiminContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, nParam);
  gsl_multimin_fdfminimizer_set(m_gslSolver, &m_gslMultiminContainer, startGuess, 0.01, 0.01);

  // for covariance matrix
  m_gslLeastSquaresContainer.f = &gsl_f;
  m_gslLeastSquaresContainer.df = &gsl_df;
  m_gslLeastSquaresContainer.fdf = &gsl_fdf;
  m_gslLeastSquaresContainer.n = nData;
  m_gslLeastSquaresContainer.p = nParam;
  m_gslLeastSquaresContainer.params = m_data;
}

void BFGS_Minimizer::initialize(API::IFitFunction* function, const std::string& costFunction) 
{
  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));

  const gsl_multimin_fdfminimizer_type *T = gsl_multimin_fdfminimizer_vector_bfgs2;

  // setup GSL container
  m_gslMultiminContainer.n = function->nActive();  
  m_gslMultiminContainer.f = &gsl_costFunction;
  m_gslMultiminContainer.df = &gsl_costFunction_df;
  m_gslMultiminContainer.fdf = &gsl_costFunction_fdf;
  m_gslMultiminContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fdfminimizer_alloc(T, function->nActive());
  gsl_multimin_fdfminimizer_set(m_gslSolver, &m_gslMultiminContainer, m_data->initFuncParams, 0.01, 0.01);

  // for covariance matrix
  m_gslLeastSquaresContainer.f = &gsl_f;
  m_gslLeastSquaresContainer.df = &gsl_df;
  m_gslLeastSquaresContainer.fdf = &gsl_fdf;
  m_gslLeastSquaresContainer.n = function->dataSize();
  m_gslLeastSquaresContainer.p = function->nActive();
  m_gslLeastSquaresContainer.params = m_data;
}

BFGS_Minimizer::~BFGS_Minimizer()
{
  delete m_data;
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
  holdCalculatedJacobian =  gsl_matrix_alloc (m_gslLeastSquaresContainer.n, m_gslLeastSquaresContainer.p);

  int dummy = m_gslLeastSquaresContainer.df(m_gslSolver->x, m_gslLeastSquaresContainer.params, holdCalculatedJacobian);
  (void) dummy;
  gsl_multifit_covar (holdCalculatedJacobian, epsrel, covar);

  gsl_matrix_free (holdCalculatedJacobian);
}

} // namespace CurveFitting
} // namespace Mantid
