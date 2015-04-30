#ifndef MANTID_CURVEFITTING_PSEUDOVOIGT_H_
#define MANTID_CURVEFITTING_PSEUDOVOIGT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {

/** PseudoVoigt

  This peak function provides an implementation of the pseudo-Voigt function,
  which is an approximation of the Voigt function (convolution of Gaussian
  and Lorentzian). The function has 4 parameters, height, FWHM, center and
  a mixing parameter a, which is limited to the interval [0,1].

  The function is defined as:

      f(x) = a * G(x) + (1.0 - a) * L(x)

  with G(x) being the Gaussian and L(x) being the Lorentzian peak function.

  This profile function is often used for peaks which are not strictly
  Gaussian or Lorentzian shaped.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 03/03/2015

  Copyright © 2015 PSI-NXMM

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
class DLLExport PseudoVoigt : public API::IPeakFunction {
public:
  virtual ~PseudoVoigt() {}

  double centre() const { return getParameter("PeakCentre"); }
  double height() const { return getParameter("Height"); }
  double fwhm() const { return getParameter("FWHM"); }

  void setCentre(const double c) { setParameter("PeakCentre", c); }
  void setHeight(const double h) { setParameter("Height", h); }
  void setFwhm(const double w) { setParameter("FWHM", w); }

  std::string name() const { return "PseudoVoigt"; }
  const std::string category() const { return "Peak"; }

protected:
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const;

  void functionDerivLocal(API::Jacobian *out, const double *xValues,
                          const size_t nData);

  void init();
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PSEUDOVOIGT_H_ */
