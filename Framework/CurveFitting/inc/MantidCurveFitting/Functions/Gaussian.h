#ifndef MANTID_CURVEFITTING_GAUSSIAN_H_
#define MANTID_CURVEFITTING_GAUSSIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
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

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Gaussian : public API::IPeakFunction {
public:
  Gaussian();
  /// overwrite IPeakFunction base class methods
  double centre() const override;
  double height() const override;
  double fwhm() const override;
  double intensity() const override;
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
  const std::string category() const override { return "Peak"; }
  void setActiveParameter(size_t i, double value) override;
  double activeParameter(size_t i) const override;

protected:
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *out, const double *xValues,
                          const size_t nData) override;
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
  /// Calculate histogram data.
  void histogram1D(double *out, double left, const double *right,
                   const size_t nBins) const override;
  /// Devivatives of the histogram.
  void histogramDerivative1D(API::Jacobian *jacobian, double left,
                             const double *right,
                             const size_t nBins) const override;
  /// Intensity cache to help recover form Sigma==0 situation
  mutable double m_intensityCache;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN_H_*/
