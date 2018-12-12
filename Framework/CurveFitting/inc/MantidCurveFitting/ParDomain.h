// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_PARDOMAIN_H_
#define MANTID_CURVEFITTING_PARDOMAIN_H_

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
  void getDomainAndValues(size_t i, API::FunctionDomain_sptr &domain,
                          API::FunctionValues_sptr &values) const override;
  /// Calculate the value of a least squares cost function
  void leastSquaresVal(
      const CostFunctions::CostFuncLeastSquares &leastSquares) override;
  /// Calculate the value, first and second derivatives of a least squares cost
  /// function
  void leastSquaresValDerivHessian(
      const CostFunctions::CostFuncLeastSquares &leastSquares, bool evalDeriv,
      bool evalHessian) override;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PARDOMAIN_H_*/
