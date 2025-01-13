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

/** ThermalNeutronBk2BkExpBETA : Function to calculate Beta of Bk2Bk Exponential
  function from
  Thermal Neutron Function's beta0, Alph1, Alph0t, Alph1t, Dtt1, and etc.
*/
class MANTID_CURVEFITTING_DLL ThermalNeutronBk2BkExpBeta : virtual public API::IFunction1D, public API::ParamFunction {
public:
  /// Override
  void function1D(double *out, const double *xValues, const size_t nData) const override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "ThermalNeutronBk2BkExpBeta"; }

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
  inline double corefunction(double dh, double width, double tcross, double beta0, double beta1, double beta0t,
                             double beta1t) const;
};

using ThermalNeutronBk2BkExpBeta_sptr = std::shared_ptr<ThermalNeutronBk2BkExpBeta>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
