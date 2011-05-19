//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidCurveFitting/CostFunctionFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCMINIMIZER(SimplexMinimizer,Simplex)

// Get a reference to the logger
Kernel::Logger& SimplexMinimizer::g_log = Kernel::Logger::get("SimplexMinimizer");

void SimplexMinimizer::initialize(double* X, const double* Y, double *sqrtWeight, 
                                   const int& nData, const int& nParam, 
                                   gsl_vector* startGuess,
                                   API::IFitFunction* function, const std::string& costFunction) 
{
  UNUSED_ARG(X);
  UNUSED_ARG(Y);
  UNUSED_ARG(sqrtWeight);
  UNUSED_ARG(nData);
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;

  // step size for simplex
  m_simplexStepSize = gsl_vector_alloc(nParam);
  gsl_vector_set_all (m_simplexStepSize, m_size);  

  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));

  // setup simplex container
  gslContainer.n = nParam;  
  gslContainer.f = &gsl_costFunction;
  gslContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fminimizer_alloc(T, nParam);
  gsl_multimin_fminimizer_set(m_gslSolver, &gslContainer, startGuess, m_simplexStepSize);

  // for covariance matrix
  m_gslLeastSquaresContainer.f = &gsl_f;
  m_gslLeastSquaresContainer.df = &gsl_df;
  m_gslLeastSquaresContainer.fdf = &gsl_fdf;
  m_gslLeastSquaresContainer.n = function->dataSize();
  m_gslLeastSquaresContainer.p = function->nActive();
  m_gslLeastSquaresContainer.params = m_data;
}

void SimplexMinimizer::initialize(API::IFitFunction* function, const std::string& costFunction) 
{
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;

  // step size for simplex
  m_simplexStepSize = gsl_vector_alloc(function->nActive());
  gsl_vector_set_all (m_simplexStepSize, m_size);  

  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(function,CostFunctionFactory::Instance().createUnwrapped(costFunction));

  // setup simplex container
  gslContainer.n = function->nActive();  
  gslContainer.f = &gsl_costFunction;
  gslContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fminimizer_alloc(T, function->nActive());
  gsl_multimin_fminimizer_set(m_gslSolver, &gslContainer, m_data->initFuncParams, m_simplexStepSize);

  m_gslLeastSquaresContainer.f = &gsl_f;
  m_gslLeastSquaresContainer.df = &gsl_df;
  m_gslLeastSquaresContainer.fdf = &gsl_fdf;
  m_gslLeastSquaresContainer.n = function->dataSize();
  m_gslLeastSquaresContainer.p = function->nActive();
  m_gslLeastSquaresContainer.params = m_data;
}

///resets the size
void SimplexMinimizer::resetSize(double* X, const double* Y, double *sqrtWeight, 
                                   const int& nData, const int& nParam, 
                                   gsl_vector* startGuess, const double& size,
                                   API::IFitFunction* function, const std::string& costFunction) 
{
  m_size = size;
  clearMemory();
  initialize(X, Y, sqrtWeight, nData, nParam, startGuess, function, costFunction);
}

///resets the size
void SimplexMinimizer::resetSize(const double& size,API::IFitFunction* function, const std::string& costFunction) 
{
  m_size = size;
  clearMemory();
  initialize(function, costFunction);
}

SimplexMinimizer::~SimplexMinimizer()
{
  clearMemory();
}

/// clear memory
void SimplexMinimizer::clearMemory()
{
  delete m_data;
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

///Calculates covariance matrix - not implemented
void SimplexMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
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
