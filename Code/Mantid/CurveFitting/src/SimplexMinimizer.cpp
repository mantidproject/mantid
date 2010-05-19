//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidKernel/Exception.h"
#include "MantidCurveFitting/GSLFunctions.h"

namespace Mantid
{
namespace CurveFitting
{

// Get a reference to the logger
Kernel::Logger& SimplexMinimizer::g_log = Kernel::Logger::get("SimplexMinimizer");

void SimplexMinimizer::initialize(double* X, const double* Y, double *sqrtWeight, 
                                   const int& nData, const int& nParam, 
                                   gsl_vector* startGuess,
                                   Fit* fit, const std::string& costFunction) 
{
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;

  // step size for simplex
  m_simplexStepSize = gsl_vector_alloc(nParam);
  gsl_vector_set_all (m_simplexStepSize, m_size);  

  // set-up GSL container to be used with GSL simplex algorithm
  m_data = new GSL_FitData(fit);  //,X, Y, sqrtWeight, nData, nParam);
  m_data->p = nParam;
  m_data->n = nData; 
  m_data->X = X;
  m_data->Y = Y;
  m_data->sqrtWeightData = sqrtWeight;
  m_data->holdCalculatedData = new double[nData];
  m_data->holdCalculatedJacobian =  gsl_matrix_alloc (nData, nParam);

  if ( costFunction.compare("Least squares") == 0 )
    m_data->costFunc = new CostFuncLeastSquares();
  else if ( costFunction.compare("Ignore positive peaks") == 0 )
    m_data->costFunc = new CostFuncIgnorePosPeaks();
  else
  {
    g_log.error("Unrecognised cost function. Default to Least squares\n");
    m_data->costFunc = new CostFuncLeastSquares();
  }

  // setup simplex container
  gslContainer.n = nParam;  
  gslContainer.f = &gsl_costFunction;
  gslContainer.params = m_data;

  // setup minimizer
  m_gslSolver = gsl_multimin_fminimizer_alloc(T, nParam);
  gsl_multimin_fminimizer_set(m_gslSolver, &gslContainer, startGuess, m_simplexStepSize);
}

void SimplexMinimizer::resetSize(double* X, const double* Y, double *sqrtWeight, 
                                   const int& nData, const int& nParam, 
                                   gsl_vector* startGuess, const double& size,
                                   Fit* fit, const std::string& costFunction) 
{
  m_size = size;
  clearMemory();
  initialize(X, Y, sqrtWeight, nData, nParam, startGuess, fit, costFunction);
}

SimplexMinimizer::~SimplexMinimizer()
{
  clearMemory();
}

/// clear memory
void SimplexMinimizer::clearMemory()
{
  delete [] m_data->holdCalculatedData;
  delete m_data->costFunc;
  gsl_matrix_free (m_data->holdCalculatedJacobian);
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

void SimplexMinimizer::calCovarianceMatrix(double epsrel, gsl_matrix * covar)
{
  Kernel::Exception::NotImplementedError("Covariance matrix calculation for Simplex not implemented.");
}

} // namespace CurveFitting
} // namespace Mantid
