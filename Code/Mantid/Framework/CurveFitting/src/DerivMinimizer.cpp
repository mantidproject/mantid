//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DerivMinimizer.h"
#include "MantidCurveFitting/CostFuncFitting.h"

namespace Mantid {
namespace CurveFitting {

/** Used by the GSL to calculate the cost function.
 * @param x :: Vector with parameters
 * @param params :: Pointer to a DerivMinimizer
 */
double DerivMinimizer::fun(const gsl_vector *x, void *params) {
  DerivMinimizer &minimizer = *static_cast<DerivMinimizer *>(params);
  size_t n = minimizer.m_costFunction->nParams();
  for (size_t i = 0; i < n; ++i) {
    minimizer.m_costFunction->setParameter(i, gsl_vector_get(x, i));
  }
  boost::shared_ptr<CostFuncFitting> fitting =
      boost::dynamic_pointer_cast<CostFuncFitting>(minimizer.m_costFunction);
  if (fitting) {
    fitting->getFittingFunction()->applyTies();
  }
  return minimizer.m_costFunction->val();
}

/** Used by the GSL to calculate the derivatives.
 * @param x :: Vector with parameters
 * @param params :: Pointer to a DerivMinimizer
 * @param g :: Buffer for the derivatives
 */
void DerivMinimizer::dfun(const gsl_vector *x, void *params, gsl_vector *g) {
  DerivMinimizer &minimizer = *static_cast<DerivMinimizer *>(params);
  size_t n = minimizer.m_costFunction->nParams();
  for (size_t i = 0; i < n; ++i) {
    minimizer.m_costFunction->setParameter(i, gsl_vector_get(x, i));
  }
  boost::shared_ptr<CostFuncFitting> fitting =
      boost::dynamic_pointer_cast<CostFuncFitting>(minimizer.m_costFunction);
  if (fitting) {
    fitting->getFittingFunction()->applyTies();
  }
  std::vector<double> der(n);
  minimizer.m_costFunction->deriv(der);
  for (size_t i = 0; i < n; ++i) {
    gsl_vector_set(g, i, der[i]);
  }
}

/** Used by the GSL to calculate the cost function and the derivatives.
 * @param x :: Vector with parameters
 * @param params :: Pointer to a DerivMinimizer
 * @param f :: Buffer for the fanction value
 * @param g :: Buffer for the derivatives
 */
void DerivMinimizer::fundfun(const gsl_vector *x, void *params, double *f,
                             gsl_vector *g) {
  DerivMinimizer &minimizer = *static_cast<DerivMinimizer *>(params);
  size_t n = minimizer.m_costFunction->nParams();
  for (size_t i = 0; i < n; ++i) {
    minimizer.m_costFunction->setParameter(i, gsl_vector_get(x, i));
  }
  boost::shared_ptr<CostFuncFitting> fitting =
      boost::dynamic_pointer_cast<CostFuncFitting>(minimizer.m_costFunction);
  if (fitting) {
    fitting->getFittingFunction()->applyTies();
  }
  std::vector<double> der(n);
  *f = minimizer.m_costFunction->valAndDeriv(der);
  for (size_t i = 0; i < n; ++i) {
    gsl_vector_set(g, i, der[i]);
  }
}

/// Constructor
DerivMinimizer::DerivMinimizer()
    : m_gslSolver(NULL), m_stopGradient(1e-3), m_stepSize(0.1),
      m_tolerance(0.0001) {}

/**
 * Constructor.
 * @param stepSize :: Initial step size.
 * @param tolerance :: Tolerance.
 */
DerivMinimizer::DerivMinimizer(const double stepSize, const double tolerance)
    : m_gslSolver(NULL), m_stopGradient(1e-3), m_stepSize(stepSize),
      m_tolerance(tolerance) {}

/**
 * Destructor.
 */
DerivMinimizer::~DerivMinimizer() {
  if (m_gslSolver != NULL) {
    gsl_multimin_fdfminimizer_free(m_gslSolver);
    gsl_vector_free(m_x);
  }
}

/**
 * Initialize the minimizer.
 * @param function :: A cost function to minimize.
 * @param maxIterations :: Maximum number of iterations.
 */
void DerivMinimizer::initialize(API::ICostFunction_sptr function,
                                size_t maxIterations) {
  UNUSED_ARG(maxIterations);
  m_costFunction = function;
  m_gslMultiminContainer.n = m_costFunction->nParams();
  m_gslMultiminContainer.f = &fun;
  m_gslMultiminContainer.df = &dfun;
  m_gslMultiminContainer.fdf = &fundfun;
  m_gslMultiminContainer.params = this;

  m_gslSolver = gsl_multimin_fdfminimizer_alloc(getGSLMinimizerType(),
                                                m_gslMultiminContainer.n);

  size_t nParams = m_costFunction->nParams();
  // Starting point
  m_x = gsl_vector_alloc(nParams);
  for (size_t i = 0; i < nParams; ++i) {
    gsl_vector_set(m_x, i, m_costFunction->getParameter(i));
  }

  gsl_multimin_fdfminimizer_set(m_gslSolver, &m_gslMultiminContainer, m_x,
                                m_stepSize, m_tolerance);
}

/**
 * Perform one iteration.
 * @return :: true to continue, false to stop.
 */
bool DerivMinimizer::iterate(size_t) {
  if (m_gslSolver == NULL) {
    throw std::runtime_error("Minimizer " + this->name() +
                             " was not initialized.");
  }
  int status = gsl_multimin_fdfminimizer_iterate(m_gslSolver);
  if (status) {
    m_errorString = gsl_strerror(status);
    return false;
  }
  status = gsl_multimin_test_gradient(m_gslSolver->gradient, m_stopGradient);
  if (status != GSL_CONTINUE) {
    m_errorString = gsl_strerror(status);
    return false;
  }
  return true;
}

/**
 * Set maximum value of the gradient at which iterations can stop
 * @param value :: New value for the gradient, must be positive.
 */
void DerivMinimizer::setStopGradient(const double value) {
  if (value <= 0) {
    throw std::invalid_argument("Gradient norm must be a positive number");
  }
  m_stopGradient = value;
}

double DerivMinimizer::costFunctionVal() { return m_gslSolver->f; }

} // namespace CurveFitting
} // namespace Mantid
