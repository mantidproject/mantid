// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/PseudoVoigt.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidKernel/make_unique.h"

#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Constraints;

using namespace API;

DECLARE_FUNCTION(PseudoVoigt)

/** Declare peak parameters in this order: mixing (eta), intensity (I), peak
 * center (x0) and FWHM (H or gamma)
 * @brief PseudoVoigt::init declare peak parameters
 */
void PseudoVoigt::init() {
  declareParameter("Mixing", 1.0);
  declareParameter("Intensity");
  declareParameter("PeakCentre");
  declareParameter("FWHM");

  auto mixingConstraint =
      Kernel::make_unique<BoundaryConstraint>(this, "Mixing", 0.0, 1.0, true);
  mixingConstraint->setPenaltyFactor(1e9);
  addConstraint(std::move(mixingConstraint));

  auto fwhm_constraint =
      Kernel::make_unique<BoundaryConstraint>(this, "FWHM", 1.E-20, true);
  fwhm_constraint->setPenaltyFactor(1e9);
  addConstraint(std::move(fwhm_constraint));
}

/** calculate pseudo voigt
 * @param out :: array with calculated value
 * @param xValues :: array with input X values
 * @param nData :: size of data
 */
void PseudoVoigt::functionLocal(double *out, const double *xValues,
                                const size_t nData) const {
  // read values
  const double peak_intensity = getParameter("Intensity");
  const double x0 = getParameter("PeakCentre");
  const double gamma = fabs(getParameter("FWHM"));
  const double gFraction = getParameter("Mixing");

  if (gamma < 1.E-20)
    throw std::runtime_error("Pseudo-voigt has FWHM as 0. It will generate "
                             "infinity at center in the Lorentzian part.");

  const double lFraction = 1.0 - gFraction;

  // calculate constants
  const double a_g = cal_ag(gamma);
  const double b_g = cal_bg(gamma);
  const double gamma_div_2 = 0.5 * gamma;
  const double gammasq_div_4 = gamma_div_2 * gamma_div_2;

  for (size_t i = 0; i < nData; ++i) {
    double xDiffSquared = (xValues[i] - x0) * (xValues[i] - x0);
    out[i] =
        peak_intensity *
        (gFraction * cal_gaussian(a_g, b_g, xDiffSquared) +
         lFraction * cal_lorentzian(gamma_div_2, gammasq_div_4, xDiffSquared));
  }
}

/** calcualte derivative analytically
 * @param out :: array with calculated derivatives
 * @param xValues :: input X array
 * @param nData :: data size
 */
void PseudoVoigt::functionDerivLocal(Jacobian *out, const double *xValues,
                                     const size_t nData) {
  // get parameters
  double peak_intensity = getParameter("Intensity");
  double x0 = getParameter("PeakCentre");
  double gamma = getParameter("FWHM");
  double gFraction = getParameter("Mixing");
  double lFraction = 1.0 - gFraction;

  // calcualte constants
  const double a_g = cal_ag(gamma);
  const double b_g = cal_bg(gamma);
  const double gamma_div_2 = 0.5 * gamma;
  const double gammasq_div_4 = gamma_div_2 * gamma_div_2;

  // derivatives
  for (size_t i = 0; i < nData; ++i) {
    double xDiff = (xValues[i] - x0);
    double xDiffSquared = xDiff * xDiff;

    // calculate normalized Gaussian and Lorentzian
    double gaussian_term = cal_gaussian(a_g, b_g, xDiffSquared);
    double lorentzian_term =
        cal_lorentzian(gamma_div_2, gammasq_div_4, xDiffSquared);

    // derivative to mixing/eta (0-th)
    out->set(i, 0, peak_intensity * (gaussian_term - lorentzian_term));

    // derivative on intensity (1-st)
    out->set(i, 1, gFraction * gaussian_term + lFraction * lorentzian_term);

    // peak center: x0
    const double derive_g_x0 = 2. * b_g * gaussian_term;
    const double derive_l_x0 =
        4. * M_PI * xDiff / gamma * lorentzian_term * lorentzian_term;
    const double deriv_x0 =
        peak_intensity * (gFraction * derive_g_x0 + lFraction * derive_l_x0);
    out->set(i, 2, deriv_x0);

    // peak width: gamma or H
    const double t1 = -1. / gamma * gaussian_term;
    const double t2 = 2. * b_g * xDiffSquared * gaussian_term / gamma;
    const double t3 = lorentzian_term / gamma;
    const double t4 = -M_PI * lorentzian_term * lorentzian_term;
    const double derive_gamma =
        peak_intensity * (gFraction * (t1 + t2) + lFraction * (t3 + t4));
    out->set(i, 3, derive_gamma);
  }
}

/** calculate effective parameter: height
 * @return
 */
double PseudoVoigt::height() const {
  double peak_intensity = getParameter("Intensity");
  double gamma = getParameter("FWHM");
  double eta = getParameter("Mixing");
  const double height =
      2. * peak_intensity * (1 + (sqrt(M_PI * M_LN2) - 1) * eta) / gamma;

  return height;
}

void PseudoVoigt::setHeight(const double h) {
  double intensity = h * 0.5; // FIXME - This is not correct!
  setParameter("Intensity", intensity);
}

/** a_G = 2/gamma * sqrt(ln2/pi)
 * @param gamma :: FWHM
 * @return
 */
double PseudoVoigt::cal_ag(const double gamma) const {
  const double ag = 2. / gamma * sqrt(M_LN2 / M_PI);
  return ag;
}

/** b_G = 4 ln2 / gamma^2
 * @param gamma :: FWHM
 * @return
 */
double PseudoVoigt::cal_bg(const double gamma) const {
  const double bg = 4 * M_LN2 / (gamma * gamma);
  return bg;
}

/** calculate normalized Gaussian
 * @param ag : a_G
 * @param bg : b_G
 * @param xdiffsq : (x - x0)**2
 * @return
 */
double PseudoVoigt::cal_gaussian(const double ag, const double bg,
                                 const double xdiffsq) const {
  const double gaussian = ag * exp(-bg * xdiffsq);
  return gaussian;
}

/** calculate lorentzian
 * @param gamma_div_2: H
 * @param gammasq_div_4: H^2
 * @param xdiffsq
 * @return
 */
double PseudoVoigt::cal_lorentzian(const double gamma_div_2,
                                   const double gammasq_div_4,
                                   const double xdiffsq) const {
  double lorentz = gamma_div_2 / (xdiffsq + gammasq_div_4) / M_PI;
  return lorentz;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
