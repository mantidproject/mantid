//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IConstraint.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

#include <sstream>

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
namespace {
/// static logger
Kernel::Logger g_log("CostFuncLeastSquares");
} // namespace

DECLARE_COSTFUNCTION(CostFuncLeastSquares, Least squares)

/**
 * Constructor
 */
CostFuncLeastSquares::CostFuncLeastSquares()
    : CostFuncFitting(), m_includePenalty(true), m_value(0), m_pushed(false),
      m_pushedValue(false), m_factor(0.5) {}

/** Calculate value of cost function
 * @return :: The value of the function
 */
double CostFuncLeastSquares::val() const {
  if (!m_dirtyVal) {
    return m_value;
  }

  checkValidity();

  m_value = 0.0;

  auto seqDomain = boost::dynamic_pointer_cast<SeqDomain>(m_domain);

  if (seqDomain) {
    seqDomain->leastSquaresVal(*this);
  } else {
    if (!m_values) {
      throw std::runtime_error("LeastSquares: undefined FunctionValues.");
    }
    addVal(m_domain, m_values);
  }

  // add penalty
  if (m_includePenalty) {
    for (size_t i = 0; i < m_function->nParams(); ++i) {
      if (!m_function->isActive(i))
        continue;
      API::IConstraint *c = m_function->getConstraint(i);
      if (c) {
        m_value += c->check();
      }
    }
  }

  m_dirtyVal = false;
  return m_value;
}

/**
 * Add a contribution to the cost function value from the fitting function
 * evaluated on a particular domain.
 * @param domain :: A domain
 * @param values :: Values
 */
void CostFuncLeastSquares::addVal(API::FunctionDomain_sptr domain,
                                  API::FunctionValues_sptr values) const {
  m_function->function(*domain, *values);
  size_t ny = values->size();

  double retVal = 0.0;

  std::vector<double> weights = getFitWeights(values);

  for (size_t i = 0; i < ny; i++) {
    double val =
        (values->getCalculated(i) - values->getFitData(i)) * weights[i];
    retVal += val * val;
  }

  PARALLEL_ATOMIC
  m_value += m_factor * retVal;
}

/** Calculate the derivatives of the cost function
 * @param der :: Container to output the derivatives
 */
void CostFuncLeastSquares::deriv(std::vector<double> &der) const {
  valDerivHessian(true, false);

  if (der.size() != nParams()) {
    der.resize(nParams());
  }
  for (size_t i = 0; i < nParams(); ++i) {
    der[i] = m_der.get(i);
  }
}

/** Calculate the value and the derivatives of the cost function
 * @param der :: Container to output the derivatives
 * @return :: The value of the function
 */
double CostFuncLeastSquares::valAndDeriv(std::vector<double> &der) const {
  valDerivHessian(true, false);

  if (der.size() != nParams()) {
    der.resize(nParams());
  }
  for (size_t i = 0; i < nParams(); ++i) {
    der[i] = m_der.get(i);
  }
  return m_value;
}

/** Calculate the value and the first and second derivatives of the cost
 * function
 *  @param evalDeriv :: flag for evaluation of the first derivatives
 *  @param evalHessian :: flag for evaluation of the second derivatives
 */
double CostFuncLeastSquares::valDerivHessian(bool evalDeriv,
                                             bool evalHessian) const {
  if (m_pushed || !evalDeriv) {
    return val();
  }

  if (!m_dirtyVal && !m_dirtyDeriv && !m_dirtyHessian) {
    return m_value;
  }

  checkValidity();

  m_value = 0.0;

  if (evalDeriv) {
    m_der.resize(nParams());
    m_der.zero();
  }
  if (evalHessian) {
    m_hessian.resize(nParams(), nParams());
    m_hessian.zero();
  }

  auto seqDomain = boost::dynamic_pointer_cast<SeqDomain>(m_domain);

  if (seqDomain) {
    seqDomain->leastSquaresValDerivHessian(*this, evalDeriv, evalHessian);
  } else {
    if (!m_values) {
      throw std::runtime_error("LeastSquares: undefined FunctionValues.");
    }
    addValDerivHessian(m_function, m_domain, m_values, evalDeriv, evalHessian);
  }

  // Add constraints penalty
  size_t np = m_function->nParams();
  if (m_includePenalty) {
    for (size_t i = 0; i < np; ++i) {
      API::IConstraint *c = m_function->getConstraint(i);
      if (c) {
        m_value += c->check();
      }
    }
  }
  m_dirtyVal = false;

  if (evalDeriv) {
    if (m_includePenalty) {
      size_t i = 0;
      for (size_t ip = 0; ip < np; ++ip) {
        if (!m_function->isActive(ip))
          continue;
        API::IConstraint *c = m_function->getConstraint(ip);
        if (c) {
          double d = m_der.get(i) + c->checkDeriv();
          m_der.set(i, d);
        }
        ++i;
      }
    }
    m_dirtyDeriv = false;
  }

  if (evalHessian) {
    if (m_includePenalty) {
      size_t i = 0;
      for (size_t ip = 0; ip < np; ++ip) {
        if (!m_function->isActive(ip))
          continue;
        API::IConstraint *c = m_function->getConstraint(ip);
        if (c) {
          double d = m_hessian.get(i, i) + c->checkDeriv2();
          m_hessian.set(i, i, d);
        }
        ++i;
      }
    }
    // clear the dirty flag if hessian was actually calculated
    m_dirtyHessian = m_hessian.isEmpty();
  }

  return m_value;
}

/**
 * Update the cost function, derivatives and hessian by adding values calculated
 * on a domain.
 * @param function :: Function to use to calculate the value and the derivatives
 * @param domain :: The domain.
 * @param values :: The fit function values
 * @param evalDeriv :: Flag to evaluate the derivatives
 * @param evalHessian :: Flag to evaluate the Hessian
 */
void CostFuncLeastSquares::addValDerivHessian(API::IFunction_sptr function,
                                              API::FunctionDomain_sptr domain,
                                              API::FunctionValues_sptr values,
                                              bool evalDeriv,
                                              bool evalHessian) const {
  UNUSED_ARG(evalDeriv);
  function->function(*domain, *values);
  size_t np = function->nParams(); // number of parameters
  size_t ny = values->size();      // number of data points
  Jacobian jacobian(ny, np);
  function->functionDeriv(*domain, jacobian);

  size_t iActiveP = 0;
  double fVal = 0.0;
  std::vector<double> weights = getFitWeights(values);

  for (size_t ip = 0; ip < np; ++ip) {
    if (!function->isActive(ip))
      continue;
    double d = 0.0;
    for (size_t i = 0; i < ny; ++i) {
      double calc = values->getCalculated(i);
      double obs = values->getFitData(i);
      double w = weights[i];
      double y = (calc - obs) * w;
      d += y * jacobian.get(i, ip) * w;
      if (iActiveP == 0) {
        fVal += y * y;
      }
    }
    PARALLEL_CRITICAL(der_set) {
      double der = m_der.get(iActiveP);
      m_der.set(iActiveP, der + d);
    }
    ++iActiveP;
  }

  PARALLEL_ATOMIC
  m_value += 0.5 * fVal;

  if (!evalHessian)
    return;

  size_t i1 = 0;                  // active parameter index
  for (size_t i = 0; i < np; ++i) // over parameters
  {
    if (!function->isActive(i))
      continue;
    size_t i2 = 0;                  // active parameter index
    for (size_t j = 0; j <= i; ++j) // over ~ half of parameters
    {
      if (!function->isActive(j))
        continue;
      double d = 0.0;
      for (size_t k = 0; k < ny; ++k) // over fitting data
      {
        double w = weights[k];
        d += jacobian.get(k, i) * jacobian.get(k, j) * w * w;
      }
      PARALLEL_CRITICAL(hessian_set) {
        double h = m_hessian.get(i1, i2);
        m_hessian.set(i1, i2, h + d);
        if (i1 != i2) {
          m_hessian.set(i2, i1, h + d);
        }
      }
      ++i2;
    }
    ++i1;
  }
}

std::vector<double>
CostFuncLeastSquares::getFitWeights(API::FunctionValues_sptr values) const {
  std::vector<double> weights(values->size());
  for (size_t i = 0; i < weights.size(); ++i) {
    weights[i] = values->getFitWeight(i);
  }

  return weights;
}

/**
 * Return cached or calculate the drivatives.
 */
const GSLVector &CostFuncLeastSquares::getDeriv() const {
  if (m_pushed) {
    return m_der;
  }
  if (m_dirtyVal || m_dirtyDeriv || m_dirtyHessian) {
    valDerivHessian();
  }
  return m_der;
}

/**
 * Return cached or calculate the Hessian.
 */
const GSLMatrix &CostFuncLeastSquares::getHessian() const {
  if (m_pushed) {
    return m_hessian;
  }
  if (m_dirtyVal || m_dirtyDeriv || m_dirtyHessian) {
    valDerivHessian();
  }
  return m_hessian;
}

/**
 * Save current parameters, derivatives and hessian.
 */
void CostFuncLeastSquares::push() {
  if (m_pushed) {
    throw std::runtime_error("Least squares: double push.");
  }
  // make sure we are not dirty
  m_pushedValue = valDerivHessian();
  getParameters(m_pushedParams);
  m_pushed = true;
}

/**
 * Restore saved parameters, derivatives and hessian.
 */
void CostFuncLeastSquares::pop() {
  if (!m_pushed) {
    throw std::runtime_error("Least squares: empty stack.");
  }
  setParameters(m_pushedParams);
  m_value = m_pushedValue;
  m_pushed = false;
  m_dirtyVal = false;
  m_dirtyDeriv = false;
  m_dirtyHessian = false;
}

/**
 * Discard saved parameters, derivatives and hessian.
 */
void CostFuncLeastSquares::drop() {
  if (!m_pushed) {
    throw std::runtime_error("Least squares: empty stack.");
  }
  m_pushed = false;
  setDirty();
}

/**
 * Calculates covariance matrix for fitting function's active parameters.
 * @param covar :: Output cavariance matrix.
 * @param epsrel :: Tolerance.
 */
void CostFuncLeastSquares::calActiveCovarianceMatrix(GSLMatrix &covar,
                                                     double epsrel) {
  UNUSED_ARG(epsrel);

  if (m_hessian.isEmpty()) {
    valDerivHessian();
  }

  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    std::ostringstream osHess;
    osHess << "== Hessian (H) ==\n";
    osHess << std::left << std::fixed;
    for (size_t i = 0; i < m_hessian.size1(); ++i) {
      for (size_t j = 0; j < m_hessian.size2(); ++j) {
        osHess << std::setw(10);
        osHess << m_hessian.get(i, j) << "  ";
      }
      osHess << "\n";
    }
    g_log.debug() << osHess.str();
  }

  covar = m_hessian;
  covar.invert();
  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    std::ostringstream osCovar;
    osCovar << "== Covariance matrix (H^-1) ==\n";
    osCovar << std::left << std::fixed;
    for (size_t i = 0; i < covar.size1(); ++i) {
      for (size_t j = 0; j < covar.size2(); ++j) {
        osCovar << std::setw(10);
        osCovar << covar.get(i, j) << "  ";
      }
      osCovar << "\n";
    }
    g_log.debug() << osCovar.str();
  }
}

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
