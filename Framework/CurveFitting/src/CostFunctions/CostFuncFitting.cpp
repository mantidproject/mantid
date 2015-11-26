// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidAPI/IConstraint.h"
#include "MantidCurveFitting/GSLJacobian.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidKernel/Exception.h"

#include <gsl/gsl_multifit_nlin.h>
#include <limits>

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {

/**
 * Constructor.
 */
CostFuncFitting::CostFuncFitting()
    : m_dirtyVal(true), m_dirtyDeriv(true), m_dirtyHessian(true),
      m_includePenalty(true), m_value(0), m_pushed(false),
      m_pushedValue(false) {}

/**
 * Set all dirty flags.
 */
void CostFuncFitting::setDirty() {
  m_dirtyVal = true;
  m_dirtyDeriv = true;
  m_dirtyHessian = true;
}

/// Get i-th parameter
/// @param i :: Index of a parameter
/// @return :: Value of the parameter
double CostFuncFitting::getParameter(size_t i) const {
  checkValidity();
  return m_function->activeParameter(m_indexMap[i]);
}

/// Set i-th parameter
/// @param i :: Index of a parameter
/// @param value :: New value of the parameter
void CostFuncFitting::setParameter(size_t i, const double &value) {
  checkValidity();
  m_function->setActiveParameter(m_indexMap[i], value);
  setDirty();
}

/// Get parameter name.
/// @param i :: Index of a cost function parameter. It may be
/// different from the index of the same parameter in the fitting
/// function.
/// @return The name of the parameter as defined by the fitting function.
std::string CostFuncFitting::parameterName(size_t i) const {
  checkValidity();
  return m_function->parameterName(m_indexMap[i]);
}

/// Number of parameters
size_t CostFuncFitting::nParams() const {
  checkValidity();
  return m_indexMap.size();
}

/** Set fitting function, domain it will operate on, and container for values.
 * @param function :: The fitting function.
 * @param domain :: The domain for the function.
 * @param values :: The FunctionValues object which receives the calculated
 * values and
 *  also contains the data to fit to and the fitting weights (reciprocal
 * errors).
 */
void CostFuncFitting::setFittingFunction(API::IFunction_sptr function,
                                         API::FunctionDomain_sptr domain,
                                         API::FunctionValues_sptr values) {
  m_function = function;
  m_domain = domain;
  m_values = values;
  reset();
}

/**
 * Is the function set and valid?
 */
bool CostFuncFitting::isValid() const {
  if (m_function == API::IFunction_sptr()) {
    return false;
  }
  if (m_function->nParams() != m_numberFunParams) {
    reset();
  }
  auto nActive = m_indexMap.size();
  for (size_t i = 0, j = 0; i < m_numberFunParams; ++i) {
    if (m_function->isActive(i)) {
      if (j >= nActive || m_indexMap[j] != i) {
        reset();
        break;
      }
      ++j;
    }
  }
  return true;
}

/**
 * Throw a runtime_error if function is invalid.
 */
void CostFuncFitting::checkValidity() const {
  if (!isValid()) {
    throw std::runtime_error("Fitting cost function isn't set");
  }
}

/**
 * Calculates covariance matrix for fitting function's active parameters.
 */
void CostFuncFitting::calActiveCovarianceMatrix(GSLMatrix &covar,
                                                double epsrel) {
  // construct the jacobian
  GSLJacobian J(*m_function, m_values->size());
  size_t na = this->nParams(); // number of active parameters
  assert(J.getJ()->size2 == na);
  covar.resize(na, na);

  // calculate the derivatives
  m_function->functionDeriv(*m_domain, J);

  // let the GSL to compute the covariance matrix
  gsl_multifit_covar(J.getJ(), epsrel, covar.gsl());
}

/** Calculates covariance matrix
 *
 * @param covar :: Returned covariance matrix
 * @param epsrel :: Is used to remove linear-dependent columns
 */
void CostFuncFitting::calCovarianceMatrix(GSLMatrix &covar, double epsrel) {
  checkValidity();
  GSLMatrix c;
  calActiveCovarianceMatrix(c, epsrel);

  size_t np = m_function->nParams();

  bool isTransformationIdentity = true;
  for (size_t i = 0; i < np; ++i) {
    if (!m_function->isActive(i))
      continue;
    isTransformationIdentity =
        isTransformationIdentity &&
        (m_function->activeParameter(i) == m_function->getParameter(i));
  }

  if (isTransformationIdentity) {
    // if the transformation is identity simply copy the matrix
    covar = c;
  } else {
    // else do the transformation
    GSLMatrix tm;
    calTransformationMatrixNumerically(tm);
    covar = tm.tr() * c * tm;
  }
}

/**
 * Calculate the fitting errors and assign them to the fitting function.
 * @param covar :: A covariance matrix to use for error calculations.
 *   It can be calculated with calCovarianceMatrix().
 * @param chi2 :: The final chi-squared of the fit.
 */
void CostFuncFitting::calFittingErrors(const GSLMatrix &covar, double chi2) {
  checkValidity();
  size_t np = m_function->nParams();
  auto covarMatrix = boost::shared_ptr<Kernel::Matrix<double>>(
      new Kernel::Matrix<double>(np, np));
  m_function->setCovarianceMatrix(covarMatrix);
  size_t ia = 0;
  for (size_t i = 0; i < np; ++i) {
    if (!m_function->isActive(i)) {
      m_function->setError(i, 0);
    } else {
      size_t ja = 0;
      for (size_t j = 0; j < np; ++j) {
        if (m_function->isActive(j)) {
          (*covarMatrix)[i][j] = covar.get(ia, ja);
          ++ja;
        }
      }
      double err = sqrt(covar.get(ia, ia));
      m_function->setError(i, err);
      ++ia;
    }
  }
  m_function->setCovarianceMatrix(covarMatrix);
  m_function->setChiSquared(chi2);
}

/**
 * Calculate the transformation matrix T by numeric differentiation
 * @param tm :: The output transformation matrix.
 */
void CostFuncFitting::calTransformationMatrixNumerically(GSLMatrix &tm) {
  const double epsilon = std::numeric_limits<double>::epsilon() * 100;
  size_t np = m_function->nParams();
  size_t na = nParams();
  tm.resize(na, na);
  size_t ia = 0;
  for (size_t i = 0; i < np; ++i) {
    if (!m_function->isActive(i))
      continue;
    double p0 = m_function->getParameter(i);
    for (size_t j = 0; j < na; ++j) {
      double ap = getParameter(j);
      double step = ap == 0.0 ? epsilon : ap * epsilon;
      setParameter(j, ap + step);
      tm.set(ia, j, (m_function->getParameter(i) - p0) / step);
      setParameter(j, ap);
    }
    ++ia;
  }
}

/// Apply ties in the fitting function
void CostFuncFitting::applyTies() {
  if (m_function) {
    m_function->applyTies();
  }
}

/// Reset the fitting function (neccessary if parameters get fixed/unfixed)
void CostFuncFitting::reset() const {
  m_numberFunParams = m_function->nParams();
  m_indexMap.clear();
  for (size_t i = 0; i < m_numberFunParams; ++i) {
    if (m_function->isActive(i)) {
      m_indexMap.push_back(i);
    }
    API::IConstraint *c = m_function->getConstraint(i);
    if (c) {
      c->setParamToSatisfyConstraint();
    }
  }
  m_dirtyDeriv = true;
  m_dirtyHessian = true;
}

/**
 * Copy the parameter values from a GSLVector.
 * @param params :: A vector to copy the parameters from
 */
void CostFuncFitting::setParameters(const GSLVector &params) {
  auto np = nParams();
  if (np != params.size()) {
    throw Kernel::Exception::FitSizeWarning(params.size(), np);
  }
  for (size_t i = 0; i < np; ++i) {
    setParameter(i, params.get(i));
  }
  m_function->applyTies();
}

/**
 * Copy the parameter values to a GSLVector.
 * @param params :: A vector to copy the parameters to
 */
void CostFuncFitting::getParameters(GSLVector &params) const {
  auto np = nParams();
  if (params.size() != np) {
    params.resize(np);
  }
  for (size_t i = 0; i < np; ++i) {
    params.set(i, getParameter(i));
  }
}

/** Calculate value of cost function
 * @return :: The value of the function
 */
double CostFuncFitting::val() const {
  if (!m_dirtyVal) {
    return m_value;
  }

  checkValidity();

  m_value = 0.0;

  auto seqDomain = boost::dynamic_pointer_cast<SeqDomain>(m_domain);

  if (seqDomain) {
    seqDomain->additiveCostFunctionVal(*this);
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

/** Calculate the derivatives of the cost function
 * @param der :: Container to output the derivatives
 */
void CostFuncFitting::deriv(std::vector<double> &der) const {
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
double CostFuncFitting::valAndDeriv(std::vector<double> &der) const {
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
double CostFuncFitting::valDerivHessian(bool evalDeriv,
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
    seqDomain->additiveCostFunctionValDerivHessian(*this, evalDeriv,
                                                   evalHessian);
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

  if (evalDeriv) {
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
 * Return cached or calculate the drivatives.
 */
const GSLVector &CostFuncFitting::getDeriv() const {
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
const GSLMatrix &CostFuncFitting::getHessian() const {
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
void CostFuncFitting::push() {
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
void CostFuncFitting::pop() {
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
void CostFuncFitting::drop() {
  if (!m_pushed) {
    throw std::runtime_error("Least squares: empty stack.");
  }
  m_pushed = false;
  setDirty();
}

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
