// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_SEQDOMAIN_H_
#define MANTID_CURVEFITTING_SEQDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IDomainCreator.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidCurveFitting/CostFunctions/CostFuncRwp.h"
#include "MantidCurveFitting/DllConfig.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace CurveFitting {
/** An implementation of CompositeDomain.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011
*/
class MANTID_CURVEFITTING_DLL SeqDomain : public API::FunctionDomain {
public:
  SeqDomain() : API::FunctionDomain(), m_currentIndex(0) {}
  /// Return the number of points in the domain
  size_t size() const override;
  /// Return the number of parts in the domain
  virtual size_t getNDomains() const;
  /// Create and return i-th domain and i-th values, (i-1)th domain is released.
  virtual void getDomainAndValues(size_t i, API::FunctionDomain_sptr &domain,
                                  API::FunctionValues_sptr &values) const;
  /// Add new domain creator
  void addCreator(API::IDomainCreator_sptr creator);
  /// Calculate the value of an additive cost function
  virtual void
  additiveCostFunctionVal(const CostFunctions::CostFuncFitting &costFunction);
  /// Calculate the value, first and second derivatives of an additive cost function.
  virtual void additiveCostFunctionValDerivHessian(
      const CostFunctions::CostFuncFitting &costFunction, bool evalDeriv,
      bool evalHessian);
  /// Calculate the value of a Rwp cost function
  void rwpVal(const CostFunctions::CostFuncRwp &rwp);
  /// Calculate the value, first and second derivatives of a RWP cost function
  void rwpValDerivHessian(const CostFunctions::CostFuncRwp &rwp, bool evalDeriv,
                          bool evalHessian);

  /// Create an instance of SeqDomain in one of two forms: either SeqDomain for
  /// sequential domain creation
  /// or ParDomain for parallel calculations
  static SeqDomain *create(API::IDomainCreator::DomainType type);

protected:
  /// Current index
  mutable size_t m_currentIndex;
  /// Currently active domain.
  mutable std::vector<API::FunctionDomain_sptr> m_domain;
  /// Currently active values.
  mutable std::vector<API::FunctionValues_sptr> m_values;
  /// Domain creators.
  std::vector<boost::shared_ptr<API::IDomainCreator>> m_creators;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_SEQDOMAIN_H_*/
