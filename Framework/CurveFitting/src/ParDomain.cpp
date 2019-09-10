// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ParDomain.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace CurveFitting {

/**
 * Create and return i-th domain and i-th values, (i-1)th domain is released.
 * @param i :: Index of domain to return.
 * @param domain :: Output pointer to the returned domain.
 * @param values :: Output pointer to the returned values.
 */
void ParDomain::getDomainAndValues(size_t i, API::FunctionDomain_sptr &domain,
                                   API::FunctionValues_sptr &values) const {
  if (i >= m_creators.size())
    throw std::range_error("Function domain index is out of range.");
  if (!m_domain[i]) {
    m_creators[i]->createDomain(m_domain[i], m_values[i]);
  }
  domain = m_domain[i];
  values = m_values[i];
}

/**
 * Calculate the value of a least squares cost function
 * @param leastSquares :: The least squares cost func to calculate the value for
 */
void ParDomain::leastSquaresVal(
    const CostFunctions::CostFuncLeastSquares &leastSquares) {
  const auto n = static_cast<int>(getNDomains());
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < n; ++i) {
    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;
    getDomainAndValues(static_cast<size_t>(i), domain, values);
    if (!values) {
      throw std::runtime_error("LeastSquares: undefined FunctionValues.");
    }
    leastSquares.addVal(domain, values);
    // PARALLEL_CRITICAL(printout)
    //{
    //  std::cerr << "val= " << leastSquares.m_value << '\n';
    //}
  }
}

/**
 * Calculate the value, first and second derivatives of a least squares cost
 * function
 * @param leastSquares :: The least squares cost func to calculate the value for
 * @param evalDeriv :: Flag to evaluate the first derivatives
 * @param evalHessian :: Flag to evaluate the Hessian (second derivatives)
 */
void ParDomain::leastSquaresValDerivHessian(
    const CostFunctions::CostFuncLeastSquares &leastSquares, bool evalDeriv,
    bool evalHessian) {
  const auto n = static_cast<int>(getNDomains());
  PARALLEL_SET_DYNAMIC(0);
  std::vector<API::IFunction_sptr> funs;
  // funs.push_back( leastSquares.getFittingFunction()->clone() );
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < n; ++i) {
    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;
    getDomainAndValues(i, domain, values);
    auto simpleValues =
        boost::dynamic_pointer_cast<API::FunctionValues>(values);
    if (!simpleValues) {
      throw std::runtime_error("LeastSquares: undefined FunctionValues.");
    }
    std::vector<API::IFunction_sptr>::size_type k = PARALLEL_THREAD_NUMBER;
    PARALLEL_CRITICAL(resize) {
      if (k >= funs.size()) {
        funs.resize(k + 1);
      }
      if (!funs[k]) {
        funs[k] = leastSquares.getFittingFunction()->clone();
      }
    }
    leastSquares.addValDerivHessian(funs[k], domain, simpleValues, evalDeriv,
                                    evalHessian);
  }
}

} // namespace CurveFitting
} // namespace Mantid
