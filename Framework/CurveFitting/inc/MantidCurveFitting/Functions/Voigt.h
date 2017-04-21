#ifndef MANTID_CURVEFITTING_VOIGT_H_
#define MANTID_CURVEFITTING_VOIGT_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

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
  std::string name() const override { return "Voigt"; }
  /// Declare parameters
  void declareParameters() override;

  /// Fill out with function values at given x points
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override;
  /// Derivatives of function with respect to active parameters
  void functionDerivLocal(API::Jacobian *out, const double *xValues,
                          const size_t nData) override;

  /// Calculate both function & derivative together
  void calculateFunctionAndDerivative(const double *xValues, const size_t nData,
                                      double *functionValues,
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

#endif /* MANTID_CURVEFITTING_VOIGT_H_ */
