// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidCurveFitting/ParDomain.h"
#include <numeric>

namespace Mantid {
namespace CurveFitting {

/// Return the number of points in the domain
size_t SeqDomain::size() const {
  return std::accumulate(m_creators.cbegin(), m_creators.cend(),
                         static_cast<size_t>(0),
                         [](size_t sum, const auto &creator) {
                           return sum + creator->getDomainSize();
                         });
}

/// Return the number of parts in the domain
size_t SeqDomain::getNDomains() const { return m_creators.size(); }

/**
 * Create and return i-th domain and i-th values, (i-1)th domain is released.
 * @param i :: Index of domain to return.
 * @param domain :: Output pointer to the returned domain.
 * @param values :: Output pointer to the returned values.
 */
void SeqDomain::getDomainAndValues(size_t i, API::FunctionDomain_sptr &domain,
                                   API::FunctionValues_sptr &values) const {
  if (i >= m_creators.size())
    throw std::range_error("Function domain index is out of range.");
  if (!m_domain[i] || i != m_currentIndex) {
    m_domain[m_currentIndex].reset();
    m_values[m_currentIndex].reset();
    m_creators[i]->createDomain(m_domain[i], m_values[i]);
    m_currentIndex = i;
  }
  domain = m_domain[i];
  values = m_values[i];
}

/**
 * Add new domain creator
 * @param creator :: A shared pointer to a new creator.
 */
void SeqDomain::addCreator(API::IDomainCreator_sptr creator) {
  m_creators.push_back(creator);
  m_domain.push_back(API::FunctionDomain_sptr());
  m_values.push_back(API::FunctionValues_sptr());
}

/**
 * Create an instance of SeqDomain in one of two forms: either SeqDomain for
 * sequential domain creation
 * or ParDomain for parallel calculations
 * @param type :: Either Sequential or Parallel
 */
SeqDomain *SeqDomain::create(API::IDomainCreator::DomainType type) {
  if (type == API::IDomainCreator::Sequential) {
    return new SeqDomain;
  } else if (type == API::IDomainCreator::Parallel) {
    return new ParDomain;
  }
  throw std::invalid_argument("Unknown SeqDomain type");
}

/**
 * Calculate the value of a least squares cost function
 * @param leastSquares :: The least squares cost func to calculate the value for
 */
void SeqDomain::leastSquaresVal(
    const CostFunctions::CostFuncLeastSquares &leastSquares) {
  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  const size_t n = getNDomains();
  for (size_t i = 0; i < n; ++i) {
    values.reset();
    getDomainAndValues(i, domain, values);
    if (!values) {
      throw std::runtime_error("LeastSquares: undefined FunctionValues.");
    }
    leastSquares.addVal(domain, values);
  }
}

//------------------------------------------------------------------------------------------------
/**
 * Calculate the value of a least squares cost function
 * @param rwp :: The RWP cost func to calculate the value for
 */
void SeqDomain::rwpVal(const CostFunctions::CostFuncRwp &rwp) {
  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  const size_t n = getNDomains();
  for (size_t i = 0; i < n; ++i) {
    values.reset();
    getDomainAndValues(i, domain, values);
    if (!values) {
      throw std::runtime_error("Rwp: undefined FunctionValues.");
    }
    rwp.addVal(domain, values);
  }
}

/**
 * Calculate the value, first and second derivatives of a least squares cost
 * function
 * @param leastSquares :: The least squares cost func to calculate the value for
 * @param evalDeriv :: Flag to evaluate the first derivatives
 * @param evalHessian :: Flag to evaluate the Hessian (second derivatives)
 */
void SeqDomain::leastSquaresValDerivHessian(
    const CostFunctions::CostFuncLeastSquares &leastSquares, bool evalDeriv,
    bool evalHessian) {
  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  const size_t n = getNDomains();
  for (size_t i = 0; i < n; ++i) {
    values.reset();
    getDomainAndValues(i, domain, values);
    if (!values) {
      throw std::runtime_error("LeastSquares: undefined FunctionValues.");
    }
    leastSquares.addValDerivHessian(leastSquares.getFittingFunction(), domain,
                                    values, evalDeriv, evalHessian);
  }
}

/**
 * Calculate the value, first and second derivatives of a RWP cost function
 * @param rwp :: The rwp cost func to calculate the value for
 * @param evalDeriv :: Flag to evaluate the first derivatives
 * @param evalHessian :: Flag to evaluate the Hessian (second derivatives)
 */
void SeqDomain::rwpValDerivHessian(const CostFunctions::CostFuncRwp &rwp,
                                   bool evalDeriv, bool evalHessian) {
  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  const size_t n = getNDomains();
  for (size_t i = 0; i < n; ++i) {
    values.reset();
    getDomainAndValues(i, domain, values);
    if (!values) {
      throw std::runtime_error("Rwp: undefined FunctionValues.");
    }
    rwp.addValDerivHessian(rwp.getFittingFunction(), domain, values, evalDeriv,
                           evalHessian);
  }
}

} // namespace CurveFitting
} // namespace Mantid
