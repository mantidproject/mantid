//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncFitting.h"
#include "MantidCurveFitting/GSLJacobian.h"
#include "MantidAPI/IConstraint.h"

#include <gsl/gsl_multifit_nlin.h>
#include <limits>

namespace Mantid {
namespace CurveFitting {

/**
 * Constructor.
 */
CostFuncFitting::CostFuncFitting()
    : m_dirtyVal(true), m_dirtyDeriv(true), m_dirtyHessian(true) {}

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
  if (domain->size() != values->size()) {
    throw std::runtime_error(
        "Function domain and values objects are incompatible.");
  }
  m_function = function;
  m_domain = domain;
  m_values = values;
  m_indexMap.clear();
  for (size_t i = 0; i < m_function->nParams(); ++i) {
    if (m_function->isActive(i)) {
      m_indexMap.push_back(i);
    }
    API::IConstraint *c = m_function->getConstraint(i);
    if (c) {
      c->setParamToSatisfyConstraint();
    }
  }
}

/**
 * Is the function set and valid?
 */
bool CostFuncFitting::isValid() const {
  return m_function != API::IFunction_sptr();
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
  GSLJacobian J(m_function, m_values->size());
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
  GSLMatrix c;
  calActiveCovarianceMatrix(c, epsrel);

  size_t np = m_function->nParams();

  bool isTransformationIdentity = true;
  size_t ii = 0;
  for (size_t i = 0; i < np; ++i) {
    if (!m_function->isActive(i))
      continue;
    isTransformationIdentity =
        isTransformationIdentity &&
        (m_function->activeParameter(i) == m_function->getParameter(i));
    ++ii;
  }

  // std::cerr << "c=\n" << c << std::endl;
  if (isTransformationIdentity) {
    // if the transformation is identity simply copy the matrix
    covar = c;
  } else {
    // else do the transformation
    GSLMatrix tm;
    calTransformationMatrixNumerically(tm);
    covar = Tr(tm) * c * tm;
    // std::cerr << "tm:\n" << tm << std::endl;
  }

  // std::cerr << "Covar:\n" << covar << std::endl;
}

/**
 * Calculate the fitting errors and assign them to the fitting function.
 * @param covar :: A covariance matrix to use for error calculations.
 *   It can be calculated with calCovarianceMatrix().
 * @param chi2 :: The final chi-squared of the fit.
 */
void CostFuncFitting::calFittingErrors(const GSLMatrix &covar, double chi2) {
  size_t np = m_function->nParams();
  auto covarMatrix = boost::shared_ptr<Kernel::Matrix<double>>(
      new Kernel::Matrix<double>(np, np));
  size_t ia = 0;
  for (size_t i = 0; i < np; ++i) {
    if (m_function->isFixed(i)) {
      m_function->setError(i, 0);
    } else {
      size_t ja = 0;
      for (size_t j = 0; j < np; ++j) {
        if (!m_function->isFixed(j)) {
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
    if (m_function->isFixed(i))
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

} // namespace CurveFitting
} // namespace Mantid
