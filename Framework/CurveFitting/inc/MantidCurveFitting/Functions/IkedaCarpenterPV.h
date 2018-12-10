// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_IKEDACARPENTERPV_H_
#define MANTID_CURVEFITTING_IKEDACARPENTERPV_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide Ikeda-Carpenter-pseudo-Voigt peak shape function interface to
IPeakFunction. See wiki
page www.mantidproject.org/IkedaCarpenterPV for documentation for this function.

@author Anders Markvardsen, ISIS, RAL
@date 3/11/2009
*/
class DLLExport IkedaCarpenterPV : virtual public API::IPeakFunction,
                                   virtual public API::IFunctionMW {
public:
  /// overwrite IPeakFunction base class methods
  double centre() const override;
  double height() const override;
  double fwhm() const override;
  void setCentre(const double c) override;
  void setHeight(const double h) override;
  void setFwhm(const double w) override;

  /// Return the peak FWHM uncertainty
  double fwhmUncertainty() const override;
  /// Return the peak intensity uncertainty
  double intensityUncertainty() const override;
  /// Return the peak height uncertainty
  double heightUncertainty() const override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "IkedaCarpenterPV"; }
  const std::string category() const override { return "Peak"; }

  /// Returns the integral intensity of the peak
  double intensity() const override;

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
  mutable std::vector<double> m_waveLength;

  /// calculate the const function
  void constFunction(double *out, const double *xValues,
                     const int &nData) const;

  /// method for updating m_waveLength
  void calWavelengthAtEachDataPoint(const double *xValues,
                                    const size_t &nData) const;

  /// convert voigt params to pseudo voigt params
  void convertVoigtToPseudo(const double &voigtSigmaSq,
                            const double &voigtGamma, double &H,
                            double &eta) const;

  /// constrain all parameters to be non-negative
  void lowerConstraint0(std::string paramName);

  size_t m_intensity_index;
  size_t m_alpha0_index;
  size_t m_alpha1_index;
  size_t m_beta0_index;
  size_t m_beta1_index;
  size_t m_kapp_index;
  size_t m_sigma2_index;
  size_t m_gamma_index;
  size_t m_center_index;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_IKEDACARPENTERPV_H_*/
