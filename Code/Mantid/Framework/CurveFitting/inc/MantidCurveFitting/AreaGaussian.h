#ifndef MANTID_CURVEFITTING_AREAGAUSSIAN_H_
#define MANTID_CURVEFITTING_AREAGAUSSIAN_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {

/** AreaGaussian : TODO: DESCRIPTION

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport AreaGaussian : public API::IPeakFunction {
public:
  virtual ~AreaGaussian() {}

  double centre() const { return getParameter("Centre"); }
  double height() const { return getParameter("Area"); }
  double fwhm() const { return getParameter("Sigma") * (2.0 * sqrt(2.0 * std::log(2.0))); }

  void setCentre(const double c) { setParameter("Centre", c); }
  void setHeight(const double h) { setParameter("Area", h); }
  void setFwhm(const double w) { setParameter("Sigma", w / (2.0 * sqrt(2.0 * std::log(2.0)))); }

  std::string name() const { return "AreaGaussian"; }
  virtual const std::string category() const { return "Peak"; }

protected:
  virtual void functionLocal(double *out, const double *xValues,
                             const size_t nData) const;

  virtual void functionDerivLocal(API::Jacobian *out, const double *xValues,
                                  const size_t nData);

  virtual void init();
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_AREAGAUSSIAN_H_ */
