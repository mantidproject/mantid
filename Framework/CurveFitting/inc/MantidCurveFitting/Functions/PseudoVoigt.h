// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

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
*/
class MANTID_CURVEFITTING_DLL PseudoVoigt : public API::IPeakFunction {
public:
  double centre() const override { return getParameter("PeakCentre"); }
  double intensity() const override { return getParameter("Intensity"); }
  double intensityError() const override { return getError("Intensity"); }
  double height() const override;
  double fwhm() const override { return getParameter("FWHM"); }

  void setCentre(const double c) override { setParameter("PeakCentre", c); }
  void setHeight(const double h) override;
  void setFwhm(const double w) override;
  void setIntensity(const double newIntensity) override { setParameter("Intensity", newIntensity); }

  std::string name() const override { return "PseudoVoigt"; }
  const std::string category() const override { return "Peak"; }

  /// Set i-th parameter
  void setParameter(size_t i, const double &value, bool explicitlySet = true) override;
  /// set by name
  void setParameter(const std::string &name, const double &value, bool explicitlySet = true) override;

protected:
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;

  void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData) override;

  void init() override;

private:
  /// historty of the order to be set
  std::vector<size_t> m_set_history_distances;
  void update_set_history(size_t set_index);
  /// get the parameter (by index) to calculate according to parameter set
  /// history
  size_t get_parameter_to_calculate_from_set();
  bool estimate_parameter_value();

  double m_height;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
