// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidCurveFitting/EigenMatrix.h"
#include "MantidCurveFitting/EigenVector.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
/** Cost function for least squares

    @author Anders Markvardsen, ISIS, RAL
    @date 11/05/2010
*/
class MANTID_CURVEFITTING_DLL CostFuncLeastSquares : public CostFuncFitting {
public:
  /// Constructor
  CostFuncLeastSquares();

  /// Get name of minimizer
  std::string name() const override { return "Least squares"; }

  /// Get short name of minimizer - useful for say labels in guis
  std::string shortName() const override { return "Chi-sq"; };

protected:
  void calActiveCovarianceMatrix(EigenMatrix &covar, double epsrel = 1e-8) override;

  void addVal(API::FunctionDomain_sptr domain, API::FunctionValues_sptr values) const override;
  void addValDerivHessian(API::IFunction_sptr function, API::FunctionDomain_sptr domain,
                          API::FunctionValues_sptr values, bool evalDeriv = true,
                          bool evalHessian = true) const override;

  /// Get mapped weights from FunctionValues
  virtual std::vector<double> getFitWeights(API::FunctionValues_sptr values) const;

  double m_factor;
};

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
