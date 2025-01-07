// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** NeutronBk2BkExpConvPVoigt : Back-to-back exponential function convoluted
  with pseudo-voigt
  for epithermal neutron TOF.

  It is the number 3 neutron TOF function of GSAS and number 9 peak profile of
  FullProf.
*/
class MANTID_CURVEFITTING_DLL NeutronBk2BkExpConvPVoigt : public API::IPowderDiffPeakFunction {

public:
  NeutronBk2BkExpConvPVoigt();

  /// Overwrite IFunction base class method: name
  std::string name() const override { return "NeutronBk2BkExpConvPVoigt"; }

  /// Overwrite IFunction base class method: category
  const std::string category() const override { return "General"; }

  /// Get peak parameters
  double getPeakParameter(const std::string &) override;

  /// Calculate peak parameters (alpha, beta, sigma2..)
  void calculateParameters(bool explicitoutput) const override;

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
  double calOmega(const double x, const double eta, const double N, const double alpha, const double beta,
                  const double H, const double sigma2, const double invert_sqrt2sigma,
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
