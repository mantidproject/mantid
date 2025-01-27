// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** ThermalNeutronBk2BkExpSIGMA : Function to calculate Sigma of Bk2Bk
  Exponential function from
  Thermal Neutron Function's Sig0, Sig1, Sig2, Width and etc.
*/
class MANTID_CURVEFITTING_DLL ThermalNeutronBk2BkExpSigma : virtual public API::IFunction1D, public API::ParamFunction {
public:
  /// Override
  void function1D(double *out, const double *xValues, const size_t nData) const override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "ThermalNeutronBk2BkExpSigma"; }

  /// Overwrite IFunction
  const std::string category() const override { return "General"; }

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// Derivative
  void functionDerivLocal(API::Jacobian *, const double *, const size_t);

  /// Derivative to overwrite
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

  /// Core function (inline) to calcualte TOF_h from d-spacing
  inline double corefunction(double dh, double sig0sq, double sig1sq, double sig2sq) const;
};

using ThermalNeutronBk2BkExpSigma_sptr = std::shared_ptr<ThermalNeutronBk2BkExpSigma>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
