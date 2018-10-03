#ifndef MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGT_H_
#define MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGT_H_

#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** NeutronBk2BkExpConvPVoigt : Back-to-back exponential function convoluted
  with pseudo-voigt
  for epithermal neutron TOF.

  It is the number 3 neutron TOF function of GSAS and number 9 peak profile of
  FullProf.

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport NeutronBk2BkExpConvPVoigt
    : public API::IPowderDiffPeakFunction {

public:
  NeutronBk2BkExpConvPVoigt();

  /// Overwrite IFunction base class method: name
  std::string name() const override { return "NeutronBk2BkExpConvPVoigt"; }

  /// Overwrite IFunction base class method: category
  const std::string category() const override { return "General"; }

  /// Get peak parameters
  double getPeakParameter(std::string) override;

  /// Calculate peak parameters (alpha, beta, sigma2..)
  void calculateParameters(bool explicitoutput) const override;

  /// Override setting a new value to the i-th parameter
  void setParameter(size_t i, const double &value,
                    bool explicitlySet = true) override;

  /// Override setting a new value to a parameter by name
  void setParameter(const std::string &name, const double &value,
                    bool explicitlySet = true) override;

  /// Set peak's height
  // virtual void setHeight(const double h);
  /// Get peak's height
  // virtual double height()const;

  using IFunction1D::function;
  void function(std::vector<double> &out,
                const std::vector<double> &xValues) const override;

  /// Function you want to fit to.
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

private:
  //----- Overwrite IFunction ------------------------------------------------
  /// Fuction local
  // void functionLocal(double* out, const double* xValues, const size_t
  // nData)const;
  /// Derivative
  // virtual void functionDerivLocal(API::Jacobian* out, const double* xValues,
  // const size_t nData);
  /// Derivative
  // virtual void functionDeriv(const API::FunctionDomain& domain,
  // API::Jacobian& jacobian);

  /// Overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// Calcualte H and Eta
  void calHandEta(double sigma2, double gamma, double &H, double &eta) const;

  /// Calculate peak profile I(TOF) = Omega(TOF)
  double calOmega(const double x, const double eta, const double N,
                  const double alpha, const double beta, const double H,
                  const double sigma2, const double invert_sqrt2sigma,
                  const bool explicitoutput = false) const;

  static int s_peakRadius;

  /// Set 2 functions to be hidden from client
  /*
  virtual void setCentre(const double c);
  virtual void setFwhm(const double w);
  */

  //------------------------------------------  Variables
  //--------------------------------------

  /// BackToBackExponential parameters
  mutable double m_Alpha;
  mutable double m_Beta;
  mutable double m_Sigma2;
  mutable double m_Gamma;

  /// FWHM
  // mutable double m_fwhm;

  /// Centre
  // mutable double m_centre;
  // mutable double m_dcentre;

  /// Thermal/Epithermal neutron related
  mutable double m_eta;
  mutable double m_N;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_NEUTRONBK2BKEXPCONVPVOIGT_H_ */
