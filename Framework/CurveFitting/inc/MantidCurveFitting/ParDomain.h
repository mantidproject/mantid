// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/SeqDomain.h"

namespace Mantid {
namespace CurveFitting {
/**
    An implementation of SeqDomain for parallel cost function and derivatives
   computation.

    @author Roman Tolchenov, Tessella plc
*/
class MANTID_CURVEFITTING_DLL ParDomain : public SeqDomain {
public:
  ParDomain() : SeqDomain() {}
  /// Create and return i-th domain and i-th values, (i-1)th domain is released.
  void getDomainAndValues(size_t i, API::FunctionDomain_sptr &domain, API::FunctionValues_sptr &values) const override;
  /// Calculate the value of an additive cost function
  void additiveCostFunctionVal(const CostFunctions::CostFuncFitting &costFunction) override;
  /// Calculate the value, first and second derivatives of an additive cost
  /// function
  void additiveCostFunctionValDerivHessian(const CostFunctions::CostFuncFitting &costFunction, bool evalDeriv,
                                           bool evalHessian) override;
};

} // namespace CurveFitting
} // namespace Mantid
