// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

#include <complex>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** Bk2BkExpConvPV : Peak profile as tback-to-back exponential convoluted with
  pseudo-Voigt.
  It is peak profile No. 10 in Fullprof.

  @date 2012-06-06
*/
class MANTID_CURVEFITTING_DLL Bk2BkExpConvPV : virtual public API::IPeakFunction, virtual public API::IFunctionMW {
public:
  /// Default constructor.
  Bk2BkExpConvPV() = default;

  /// overwrite IPeakFunction base class methods
  double centre() const override;
  double height() const override;
  double fwhm() const override;
  void setCentre(const double c) override;
  void setHeight(const double h) override;
  void setFwhm(const double w) override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "Bk2BkExpConvPV"; }
  const std::string category() const override { return "Peak"; }

  /// Set up the range of peak calculation for higher efficiency
  // void setCalculationRange(double tof_low, double tof_upper);
  /// Calculate peak
  void geneatePeak(double *out, const double *xValues, const size_t nData);

  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                          double endX) override;

protected:
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData) override;
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// container for storing wavelength values for each data point
  mutable std::vector<double> m_dtt1;

  double calOmega(double x, double eta, double N, double alpha, double beta, double H, double sigma2,
                  double invert_sqrt2sigma) const;

  std::complex<double> E1(std::complex<double> z) const;

  void calHandEta(double sigma2, double gamma, double &H, double &eta) const;
  double expWidth() const;
};

// using TableWorkspace_sptr = std::shared_ptr<TableWorkspace>;

using Bk2BkExpConvPV_sptr = std::shared_ptr<Bk2BkExpConvPV>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
