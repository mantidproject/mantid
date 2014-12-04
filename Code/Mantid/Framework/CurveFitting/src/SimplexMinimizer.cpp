//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidCurveFitting/CostFuncFitting.h"

#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{
  namespace
  {
    /// static logger
    Kernel::Logger g_log("SimplexMinimizer");
  }

DECLARE_FUNCMINIMIZER(SimplexMinimizer,Simplex)


/** Calculating cost function
*
* @param x :: Input function arguments
* @param params :: Pointer to a SimplexMinimizer
* @return Value of the cost function
*/
double SimplexMinimizer::fun(const gsl_vector * x, void *params)
{
  SimplexMinimizer& minimizer = *static_cast<SimplexMinimizer*>(params);
  // update function parameters
  if (x->data)
  {
    for(size_t i = 0; i < minimizer.m_costFunction->nParams(); ++i)
    {
      minimizer.m_costFunction->setParameter(i, gsl_vector_get(x,i));
    }
  }
  boost::shared_ptr<CostFuncFitting> fitting = boost::dynamic_pointer_cast<CostFuncFitting>(minimizer.m_costFunction);
  if (fitting)
  {
    fitting->getFittingFunction()->applyTies();
  }
  return minimizer.m_costFunction->val();
}


SimplexMinimizer::SimplexMinimizer(const double epsabs):
m_epsabs(epsabs),
m_costFunction(),
m_size(1.0),
m_simplexStepSize(NULL),
m_startGuess(NULL),
m_gslSolver(NULL)
{
}

void SimplexMinimizer::initialize(API::ICostFunction_sptr function, size_t)
{
  m_costFunction = function;

  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex;

  size_t np = function->nParams();
  // step size for simplex
  m_simplexStepSize = gsl_vector_alloc(np);
  gsl_vector_set_all (m_simplexStepSize, m_size);  

  // setup simplex container
  gslContainer.n = np;
  gslContainer.f = &fun;
  gslContainer.params = this;

  // fill in parameter values
  m_startGuess = gsl_vector_alloc(np);
  for (size_t i = 0; i < np; ++i)
  {
    gsl_vector_set(m_startGuess,i,function->getParameter(i));
  }

  // setup minimizer
  m_gslSolver = gsl_multimin_fminimizer_alloc(T, np);
  gsl_multimin_fminimizer_set(m_gslSolver, &gslContainer, m_startGuess, m_simplexStepSize);

}

/**
 * Do one iteration.
 * @return :: true if iterations to be continued, false if they can stop
 */
bool SimplexMinimizer::iterate(size_t)
{
  int status = gsl_multimin_fminimizer_iterate(m_gslSolver);
  if (status)
  {
    m_errorString = gsl_strerror(status);
    return false;
  }
  double size = gsl_multimin_fminimizer_size(m_gslSolver);
  status = gsl_multimin_test_size(size,m_epsabs);
  if (status != GSL_CONTINUE)
  {
    m_errorString = gsl_strerror(status);
    return false;
  }
  return true;
}

///resets the size
void SimplexMinimizer::resetSize(const double& size) 
{
  m_size = size;
  clearMemory();
  initialize(m_costFunction);
}

SimplexMinimizer::~SimplexMinimizer()
{
  clearMemory();
}

/// clear memory
void SimplexMinimizer::clearMemory()
{
  if ( m_simplexStepSize )
  {
    gsl_vector_free(m_simplexStepSize);
  }
  if ( m_startGuess )
  {
    gsl_vector_free(m_startGuess);
  }
  if ( m_gslSolver )
  {
    gsl_multimin_fminimizer_free(m_gslSolver);
  }
}

double SimplexMinimizer::costFunctionVal()
{
  return m_gslSolver->fval;
}

} // namespace CurveFitting
} // namespace Mantid
