// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

#include <complex>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** ThermalNeutronBk2BkExpConvPVoigt :
    Back-to-back exponential convoluted with pseudo Voigt for thermal neutron
  and epithermal neutron TOF

    It will involve the calculation from peak's miller indices
*/

class MANTID_CURVEFITTING_DLL ThermalNeutronBk2BkExpConvPVoigt : public API::IPowderDiffPeakFunction {
public:
  ThermalNeutronBk2BkExpConvPVoigt();

  /// Overwrite IFunction base class methods
  std::string name() const override { return "ThermalNeutronBk2BkExpConvPVoigt"; }
  const std::string category() const override { return "General"; }

  /// Overwrite IPeakFunction base class methods
  /*
  virtual double centre()const;

  virtual double fwhm()const;
  virtual void setHeight(const double h);
  virtual void setPeakRadius(const int& r);
  */

  //--------------- ThermalNeutron peak function special
  //---------------------------------------
  /// Set Miller Indicies
  // virtual void setMillerIndex(int h, int k, int l);

  /// Get Miller Index from this peak
  // virtual void getMillerIndex(int& h, int &k, int &l);

  /// Get peak parameters
  double getPeakParameter(const std::string &) override;

  /// Calculate peak parameters (alpha, beta, sigma2..)
  void calculateParameters(bool explicitoutput) const override;

  /// Core function to calcualte peak values for whole region
  // void functionLocal(vector<double>& out, const vector<double> &xValues)
  // const;

  /// Set up the flag to show whether (from client) cell parameter value changed
  /*
  void setUnitCellParameterValueChangeFlag(bool changed)
  {
    m_cellParamValueChanged = changed;
  }
  */

  /// Override setting a new value to the i-th parameter
  void setParameter(size_t i, const double &value, bool explicitlySet = true) override;

  /// Override setting a new value to a parameter by name
  void setParameter(const std::string &name, const double &value, bool explicitlySet = true) override;

  /// Set peak's height
  // virtual void setHeight(const double h);
  /// Get peak's height
  // virtual double height()const;

  using IFunction1D::function;
  void function(std::vector<double> &out, const std::vector<double> &xValues) const override;

  /// Function you want to fit to.
  void function1D(double *out, const double *xValues, const size_t nData) const override;

private:
  //----- Overwrite IFunction ------------------------------------------------
  /// Fuction local
  void functionLocal(double *out, const double *xValues, const size_t nData) const;
  /// Derivative
  virtual void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData);
  /// Derivative
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

  /// Overwrite IFunction base class method, which declare function parameters
  void init() override;

  static int s_peakRadius;

  //--------  Private Functions -----------------------------------
  /// Calcualte H and Eta
  void calHandEta(double sigma2, double gamma, double &H, double &eta) const;

  /// Calculate peak center
  double calPeakCenter() const;

  /// Calculate peak profile I(TOF) = Omega(TOF)
  double calOmega(const double x, const double eta, const double N, const double alpha, const double beta,
                  const double H, const double sigma2, const double invert_sqrt2sigma,
                  const bool explicitoutput = false) const;

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

  /// Override setting a new value to the

  //-----------  For Parallelization -----------------------------------------
  ///
  void interruption_point() const;
  /// Set to true to stop execution
  mutable bool m_cancel;
  /// Set if an exception is thrown, and not caught, within a parallel region
  mutable bool m_parallelException;
  /// Flag to show whether the unit cell has been calcualted
  mutable bool m_dspaceCalculated;

  /// Flag to indicate whether there is new parameter value set after
  /// calculating parameters
  // mutable bool m_newValueSet;
};

/// Shared pointer to ThermalNeutronBk2BkExpConvPVoigt peak/function
using ThermalNeutronBk2BkExpConvPVoigt_sptr = std::shared_ptr<ThermalNeutronBk2BkExpConvPVoigt>;

//--- Public inline function --------------------------------------------------
/** Calculate d = a/sqrt(h**2+k**2+l**2)
inline double calCubicDSpace(double a, int h, int k, int l)
{
    // TODO This function will be refactored in future.
    double d = a/( sqrt(double(h*h)+double(k*k)+double(l*l)) );

    return d;
}
*/

/// Integral for Gamma
// std::complex<double> E1X(std::complex<double> z);

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
