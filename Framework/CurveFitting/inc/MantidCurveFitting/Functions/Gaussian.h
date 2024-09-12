// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide gaussian peak shape function interface to IPeakFunction.
I.e. the function: Height*exp(-0.5*((x-PeakCentre)/Sigma)^2).

This function actually performs the fitting on 1/Sigma^2 rather than Sigma
for stability reasons.

Gauassian parameters:
<UL>
<LI> Height - height of peak (default 0.0)</LI>
<LI> PeakCentre - centre of peak (default 0.0)</LI>
<LI> Sigma - standard deviation (default 0.0)</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 19/10/2009
*/
class MANTID_CURVEFITTING_DLL Gaussian : public API::IPeakFunction {
public:
  Gaussian();
  /// overwrite IPeakFunction base class methods
  double centre() const override;
  std::string getWidthParameterName() const override { return "Sigma"; }
  double height() const override;
  double fwhm() const override;
  double intensity() const override;
  double intensityError() const override;
  void setCentre(const double c) override;
  void setHeight(const double h) override;
  void setFwhm(const double w) override;
  void setIntensity(const double i) override;

  void fixCentre(bool isDefault = false) override;
  void unfixCentre() override;
  void fixIntensity(bool isDefault = false) override;
  void unfixIntensity() override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "Gaussian"; }
  const std::string category() const override { return "Peak; Muon\\MuonModelling"; }
  void setActiveParameter(size_t i, double value) override;
  double activeParameter(size_t i) const override;

protected:
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData) override;
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
  /// Calculate histogram data.
  void histogram1D(double *out, double left, const double *right, const size_t nBins) const override;
  /// Devivatives of the histogram.
  void histogramDerivative1D(API::Jacobian *jacobian, double left, const double *right,
                             const size_t nBins) const override;
  /// Intensity cache to help recover form Sigma==0 situation
  mutable double m_intensityCache;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
