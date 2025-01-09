// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

#include <cmath>
#include <gsl/gsl_sf_erf.h>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** ThermalNeutronDtoTOFFunction : TODO: DESCRIPTION
 */
class MANTID_CURVEFITTING_DLL ThermalNeutronDtoTOFFunction : virtual public API::IFunction1D,
                                                             public API::ParamFunction {
public:
  /// Override
  void function1D(double *out, const double *xValues, const size_t nData) const override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "ThermalNeutronDtoTOFFunction"; }

  /// Overwrite IFunction
  const std::string category() const override { return "General"; }

  /// Calculate function values
  void function1D(std::vector<double> &out, const std::vector<double> &xValues) const;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// Core function (inline) to calcualte TOF_h from d-spacing
  inline double corefunction(double dh, double dtt1, double dtt1t, double dtt2t, double zero, double zerot,
                             double width, double tcross) const;

  /// Derivative
  void functionDerivLocal(API::Jacobian *, const double *, const size_t);

  /// Derivative
  // void functionDeriv(const API::FunctionDomain& domain, API::Jacobian&
  // jacobian);

  /// Derviate to overwritten
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  // void functionDeriv(const API::FunctionDomain& domain, API::Jacobian&
  // jacobian);
};

using ThermalNeutronDtoTOFFunction_sptr = std::shared_ptr<ThermalNeutronDtoTOFFunction>;

/// Calcualte TOF from d-spacing value for thermal neutron
inline double calThermalNeutronTOF(double dh, double dtt1, double dtt1t, double dtt2t, double zero, double zerot,
                                   double width, double tcross) {
  double n = 0.5 * gsl_sf_erfc(width * (tcross - 1 / dh));
  double Th_e = zero + dtt1 * dh;
  double Th_t = zerot + dtt1t * dh - dtt2t / dh;
  double tof_h = n * Th_e + (1 - n) * Th_t;

  return tof_h;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
