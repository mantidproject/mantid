// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {

/** CostFuncPoisson : Implements a cost function for fitting applications using
  a Poisson measure
*/
class MANTID_CURVEFITTING_DLL CostFuncPoisson : public CostFuncFitting {
public:
  CostFuncPoisson();
  /// Get name of minimizer
  virtual std::string name() const override { return "Poisson"; }
  /// Get short name of minimizer - useful for say labels in guis
  virtual std::string shortName() const override { return "Poisson"; };

  void addVal(API::FunctionDomain_sptr domain, API::FunctionValues_sptr values) const override;
  void addValDerivHessian(API::IFunction_sptr function, API::FunctionDomain_sptr domain,
                          API::FunctionValues_sptr values, bool evalDeriv = true,
                          bool evalHessian = true) const override;

private:
  /// Calculates the derivative for the addValDerivHessian method
  void calculateDerivative(API::IFunction &function, API::FunctionDomain &domain, API::FunctionValues &values) const;
  /// Calculates the Hessian matrix for the addValDerivHessian method
  void calculateHessian(API::IFunction &function, API::FunctionDomain &domain, API::FunctionValues &values) const;
};

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
