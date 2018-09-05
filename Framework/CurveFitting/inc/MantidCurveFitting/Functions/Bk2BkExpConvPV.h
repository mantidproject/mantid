#ifndef MANTID_CURVEFITTING_BK2BKEXPCONVPV_H_
#define MANTID_CURVEFITTING_BK2BKEXPCONVPV_H_

#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/System.h"
#include <complex>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** Bk2BkExpConvPV : Peak profile as tback-to-back exponential convoluted with
  pseudo-Voigt.
  It is peak profile No. 10 in Fullprof.

  @date 2012-06-06

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
class DLLExport Bk2BkExpConvPV : virtual public API::IPeakFunction,
                                 virtual public API::IFunctionMW {
public:
  Bk2BkExpConvPV();

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
  ///
  void resetFWHM();

protected:
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *out, const double *xValues,
                          const size_t nData) override;
  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian) override;

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// container for storing wavelength values for each data point
  mutable std::vector<double> m_dtt1;

  double calOmega(double x, double eta, double N, double alpha, double beta,
                  double H, double sigma2, double invert_sqrt2sigma) const;

  std::complex<double> E1(std::complex<double> z) const;

  void calHandEta(double sigma2, double gamma, double &H, double &eta) const;

  mutable double mFWHM;
  mutable double mLowTOF;
  mutable double mUpperTOF;
};

// typedef boost::shared_ptr<TableWorkspace> TableWorkspace_sptr;

using Bk2BkExpConvPV_sptr = boost::shared_ptr<Bk2BkExpConvPV>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_BK2BKEXPCONVPV_H_ */
