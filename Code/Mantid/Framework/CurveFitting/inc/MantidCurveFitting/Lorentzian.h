#ifndef MANTID_CURVEFITTING_LORENTZIAN_H_
#define MANTID_CURVEFITTING_LORENTZIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {
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
  /// Destructor
  virtual ~Lorentzian(){};

  /// overwrite IPeakFunction base class methods
  virtual double centre() const { return getParameter("PeakCentre"); }
  virtual double height() const;
  virtual double fwhm() const { return getParameter("FWHM"); }
  virtual void setCentre(const double c) { setParameter("PeakCentre", c); }
  virtual void setHeight(const double h);
  virtual void setFwhm(const double w) { setParameter("FWHM", w); }

  /// overwrite IFunction base class methods
  std::string name() const { return "Lorentzian"; }
  virtual const std::string category() const { return "Peak"; }

protected:
  virtual void functionLocal(double *out, const double *xValues,
                             const size_t nData) const;
  virtual void functionDerivLocal(API::Jacobian *out, const double *xValues,
                                  const size_t nData);
  /// overwrite IFunction base class method, which declare function parameters
  virtual void init();
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LORENTZIAN_H_*/
