//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DerivMinimizer.h"

namespace Mantid
{
namespace CurveFitting
{

double DerivMinimizer::fun (const gsl_vector * x, void * params)
{
  DerivMinimizer& minimizer = *static_cast<DerivMinimizer*>(params);
  size_t n = minimizer.m_costFunction->nParams();
  for(size_t i = 0;i < n; ++i)
  {
    minimizer.m_costFunction->setParameter(i,gsl_vector_get(x, i));
  }
  return minimizer.m_costFunction->val();
}

void DerivMinimizer::dfun(const gsl_vector * x, void * params, gsl_vector * g)
{
  DerivMinimizer& minimizer = *static_cast<DerivMinimizer*>(params);
  size_t n = minimizer.m_costFunction->nParams();
  for(size_t i = 0;i < n; ++i)
  {
    minimizer.m_costFunction->setParameter(i,gsl_vector_get(x, i));
  }
  std::vector<double> der(n);
  minimizer.m_costFunction->deriv(der);
  for(size_t i = 0;i < n; ++i)
  {
    gsl_vector_set(g, i, der[i]);
  }
}

void DerivMinimizer::fundfun (const gsl_vector * x, void * params, double * f, gsl_vector * g)
{
  DerivMinimizer& minimizer = *static_cast<DerivMinimizer*>(params);
  size_t n = minimizer.m_costFunction->nParams();
  for(size_t i = 0;i < n; ++i)
  {
    minimizer.m_costFunction->setParameter(i,gsl_vector_get(x, i));
  }
  std::vector<double> der(n);
  *f = minimizer.m_costFunction->valAndDeriv(der);
  for(size_t i = 0;i < n; ++i)
  {
    gsl_vector_set(g, i, der[i]);
  }
}

/// Constructor
DerivMinimizer::DerivMinimizer():
m_gslSolver(NULL)
{
}

DerivMinimizer::~DerivMinimizer()
{
  if (m_gslSolver != NULL)
  {
    gsl_multimin_fdfminimizer_free(m_gslSolver);
    gsl_vector_free(m_x);
  }
}

void DerivMinimizer::initialize(API::ICostFunction_sptr function) 
{
  m_costFunction = function;
  m_gslMultiminContainer.n = m_costFunction->nParams();
  m_gslMultiminContainer.f = &fun;
  m_gslMultiminContainer.df = &dfun;
  m_gslMultiminContainer.fdf = &fundfun;
  m_gslMultiminContainer.params = this;

  gsl_multimin_fdfminimizer *s = gsl_multimin_fdfminimizer_alloc( getGSLMinimizerType(), m_gslMultiminContainer.n );

  size_t nParams = m_costFunction->nParams();
  // Starting point 
  m_x = gsl_vector_alloc (nParams);
  for(size_t i = 0; i < nParams; ++i)
  {
    gsl_vector_set (m_x, i, m_costFunction->getParameter(i));
  }

  gsl_multimin_fdfminimizer_set (m_gslSolver, &m_gslMultiminContainer, m_x, 0.01, 1e-4);

}

/**
 */
bool DerivMinimizer::iterate() 
{
  if (m_gslSolver == NULL)
  {
    throw std::runtime_error("Minimizer " + this->name() + " was not initialized.");
  }
  int status = gsl_multimin_fdfminimizer_iterate(m_gslSolver);
  if (status) return false;
  status = gsl_multimin_test_gradient (m_gslSolver->gradient, 1e-3); //! <---------------
  return status == GSL_CONTINUE;
}

bool DerivMinimizer::minimize() 
{

  size_t iter = 0;
  bool success = false;
  do
  {
    iter++;
    if ( !iterate() )
    {
      success = true;
      break;
    }
  }
  while (iter < 100); //! <---------------

  return success;

}

double DerivMinimizer::costFunctionVal()
{
  return m_gslSolver->f;
}

void DerivMinimizer::calCovarianceMatrix(gsl_matrix * covar, double epsrel)
{
  //gsl_matrix * holdCalculatedJacobian;
  //holdCalculatedJacobian =  gsl_matrix_alloc (m_gslLeastSquaresContainer.n, m_gslLeastSquaresContainer.p);

  //int dummy = m_gslLeastSquaresContainer.df(m_gslSolver->x, m_gslLeastSquaresContainer.params, holdCalculatedJacobian);
  //(void) dummy;
  //gsl_multifit_covar (holdCalculatedJacobian, epsrel, covar);

  //gsl_matrix_free (holdCalculatedJacobian);
}

} // namespace CurveFitting
} // namespace Mantid
