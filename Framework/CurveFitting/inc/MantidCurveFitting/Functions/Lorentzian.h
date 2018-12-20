#ifndef MANTID_CURVEFITTING_LORENTZIAN_H_
#define MANTID_CURVEFITTING_LORENTZIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide lorentzian peak shape function interface to IPeakFunction.
I.e. the function: \f$ \frac{A}{\pi}( \Gamma/2((x-PeakCentre)^2+(\Gamma/2)^2)
)\f$.

\f$\Gamma/2\f$ (HWHM) - half-width at half-maximum

Lorentzian parameters:
<UL>
<LI> Amplitude - Intensity scaling (default 1.0)</LI>
<LI> PeakCentre - centre of peak (default 0.0)</LI>
<LI> FWHM - Full-width half-maximum (default 0.0)</LI>
</UL>

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
class DLLExport Lorentzian : public API::IPeakFunction {
public:
  Lorentzian();
  /// overwrite IPeakFunction base class methods
  double centre() const override { return getParameter("PeakCentre"); }
  double height() const override;
  double fwhm() const override { return getParameter("FWHM"); }
  double intensity() const override { return getParameter("Amplitude"); }
  void setCentre(const double c) override { setParameter("PeakCentre", c); }
  void setHeight(const double h) override;
  void setFwhm(const double w) override;
  void setIntensity(const double i) override { setParameter("Amplitude", i); }
  void fixCentre(bool isDefault = false) override;
  void unfixCentre() override;
  void fixIntensity(bool isDefault = false) override;
  void unfixIntensity() override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "Lorentzian"; }
  const std::string category() const override { return "Peak"; }

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

private:
  /// When Amplitude is set via setHeight() and fwhm() == 0 height is made equal
  /// to Amplitude.
  /// The flag is used after that when FWHM is set to non-zero value.
  /// The height in this case must remain the same but amplitude change.
  bool m_amplitudeEqualHeight;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LORENTZIAN_H_*/
