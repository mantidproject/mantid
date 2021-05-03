// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  Implements an analytical approximation to the Voigt function.
  See http://www.citeulike.org/user/dbomse/article/9553243 for approximation
  used
 */
class MANTID_CURVEFITTING_DLL Voigt : public API::IPeakFunction {
private:
  /// Return a string identifier for the function
  std::string name() const override { return "Voigt"; }
  /// Declare parameters
  void declareParameters() override;

  /// Fill out with function values at given x points
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;
  /// Derivatives of function with respect to active parameters
  void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData) override;

  /// Calculate both function & derivative together
  void calculateFunctionAndDerivative(const double *xValues, const size_t nData, double *functionValues,
                                      API::Jacobian *derivatives) const;

  /// Return value of centre of peak
  double centre() const override;
  /// Return value of height of peak
  double height() const override;
  /// Return value of FWHM of peak
  double fwhm() const override;
  /// Set the centre of the peak
  void setCentre(const double value) override;
  /// Set the height of the peak
  void setHeight(const double value) override;
  /// Set the FWHM of the peak
  void setFwhm(const double value) override;
  /// Returns the integral intensity of the peak
  double intensity() const override;
  /// Sets the integral intensity of the peak
  void setIntensity(const double value) override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
