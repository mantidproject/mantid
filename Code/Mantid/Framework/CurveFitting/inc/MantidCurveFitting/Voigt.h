#ifndef MANTID_CURVEFITTING_VOIGT_H_
#define MANTID_CURVEFITTING_VOIGT_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {

/**
  Implements an analytical approximation to the Voigt function.
  See http://www.citeulike.org/user/dbomse/article/9553243 for approximation
  used

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_CURVEFITTING_DLL Voigt : public API::IPeakFunction {
private:
  /// Return a string identifier for the function
  std::string name() const { return "Voigt"; }
  /// Declare parameters
  void declareParameters();

  /// Fill out with function values at given x points
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const;
  /// Derivatives of function with respect to active parameters
  void functionDerivLocal(API::Jacobian *out, const double *xValues,
                          const size_t nData);

  /// Calculate both function & derivative together
  void calculateFunctionAndDerivative(const double *xValues, const size_t nData,
                                      double *functionValues,
                                      API::Jacobian *derivatives) const;

  /// Return value of centre of peak
  double centre() const;
  /// Return value of height of peak
  double height() const;
  /// Return value of FWHM of peak
  double fwhm() const;
  /// Set the centre of the peak
  void setCentre(const double value);
  /// Set the height of the peak
  void setHeight(const double value);
  /// Set the FWHM of the peak
  void setFwhm(const double value);
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_VOIGT_H_ */
