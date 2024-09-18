// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/PseudoVoigt.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"

#include <iostream>

#include <cmath>

namespace {

const double LN2_DIV_PI = 2. * std::sqrt(M_LN2 / M_PI);
constexpr double LN2_M4 = 4 * M_LN2;

/** a_G = 2/gamma * sqrt(ln2/pi)
 * @param gamma :: FWHM
 * @return
 */
inline double cal_ag(const double gamma) { return LN2_DIV_PI / gamma; }

/** b_G = 4 ln2 / gamma^2
 * @param gamma :: FWHM
 * @return
 */
inline constexpr double cal_bg(const double gamma) { return LN2_M4 / (gamma * gamma); }

/** calculate normalized Gaussian
 * @param ag : a_G
 * @param bg : b_G
 * @param xdiffsq : (x - x0)**2
 * @return
 */
inline double cal_gaussian(const double ag, const double bg, const double xdiffsq) {
  return ag * std::exp(-bg * xdiffsq);
}

/** calculate lorentzian
 * @param gamma_div_2: H
 * @param gammasq_div_4: H^2
 * @param xdiffsq
 * @return
 */
inline constexpr double cal_lorentzian(const double gamma_div_2, const double gammasq_div_4, const double xdiffsq) {
  return gamma_div_2 / (xdiffsq + gammasq_div_4) / M_PI;
}

} // namespace

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
using namespace Constraints;
using namespace std;
using namespace API;

namespace {
/// static logger
Kernel::Logger g_log("PseudoVoigt");
} // namespace

DECLARE_FUNCTION(PseudoVoigt)

/** Declare peak parameters in this order: mixing (eta), intensity (I), peak
 * center (x0) and FWHM (H or gamma)
 * @brief PseudoVoigt::init declare peak parameters
 */
void PseudoVoigt::init() {
  declareParameter("Mixing", 0.5);
  declareParameter("Intensity", 1.);
  declareParameter("PeakCentre", 0.);
  declareParameter("FWHM", 1.);

  auto mixingConstraint = std::make_unique<BoundaryConstraint>(this, "Mixing", 0.0, 1.0, true);
  mixingConstraint->setPenaltyFactor(1e9);
  addConstraint(std::move(mixingConstraint));

  auto fwhm_constraint = std::make_unique<BoundaryConstraint>(this, "FWHM", 1.E-20, true);
  fwhm_constraint->setPenaltyFactor(1e9);
  addConstraint(std::move(fwhm_constraint));

  // init the peak height setup parameters
  m_height = 1.; // peak height set by user

  // parameter set history: all start from an arbitary out of boundary value
  // (100 as easy). if set, order set to 0
  m_set_history_distances.resize(4, 128);
  // 0: mixing, 1: intensity, 2: fwhm, 3: height
}

/** calculate pseudo voigt
 * @param out :: array with calculated value
 * @param xValues :: array with input X values
 * @param nData :: size of data
 */
void PseudoVoigt::functionLocal(double *out, const double *xValues, const size_t nData) const {
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
    out[i] = peak_intensity * (gFraction * cal_gaussian(a_g, b_g, xDiffSquared) +
                               lFraction * cal_lorentzian(gamma_div_2, gammasq_div_4, xDiffSquared));
  }
}

/** calcualte derivative analytically
 * @param out :: array with calculated derivatives
 * @param xValues :: input X array
 * @param nData :: data size
 */
void PseudoVoigt::functionDerivLocal(Jacobian *out, const double *xValues, const size_t nData) {
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
    double lorentzian_term = cal_lorentzian(gamma_div_2, gammasq_div_4, xDiffSquared);

    // derivative to mixing/eta (0-th)
    out->set(i, 0, peak_intensity * (gaussian_term - lorentzian_term));

    // derivative on intensity (1-st)
    out->set(i, 1, gFraction * gaussian_term + lFraction * lorentzian_term);

    // peak center: x0
    const double derive_g_x0 = 2. * b_g * xDiff * gaussian_term;
    const double derive_l_x0 = 4. * M_PI * xDiff / gamma * lorentzian_term * lorentzian_term;
    const double deriv_x0 = peak_intensity * (gFraction * derive_g_x0 + lFraction * derive_l_x0);
    out->set(i, 2, deriv_x0);

    // peak width: gamma or H
    const double t1 = -1. / gamma * gaussian_term;
    const double t2 = 2. * b_g * xDiffSquared * gaussian_term / gamma;
    const double t3 = lorentzian_term / gamma;
    const double t4 = -M_PI * lorentzian_term * lorentzian_term;
    const double derive_gamma = peak_intensity * (gFraction * (t1 + t2) + lFraction * (t3 + t4));
    out->set(i, 3, derive_gamma);
  }
}

/// override because setParameter(size_t i ...) is overriden
void PseudoVoigt::setParameter(const std::string &name, const double &value, bool explicitlySet) {
  g_log.debug() << "[PV] Set " << name << " as " << value << "\n";
  API::IPeakFunction::setParameter(name, value, explicitlySet);
}

/** set parameter by parameter index
 * Parameter set up scenaio:
 * - as both intensity and height are set explicity: calculate mixing and set
 * non-explicitly
 * - as both intensity and height are set but intensity is not set explicitly:
 * do nothing else
 * - if both height and mixing are specified explicitly, then calculate peak
 * intensity
 * Note: both intensity and height
 * Equation among eta, I, H and height:
 * peak_intensity =
 *    m_height / 2. / (1 + (sqrt(M_PI * M_LN2) - 1) * eta) * (M_PI * gamma);
 * @param i :: index (mixing (0), intensity (1), centre (2), fwhm (3)
 * @param value :: value
 * @param explicitlySet :: whether it is an explicit set.  if true, then
 * consider to calculate other parameters
 */
void PseudoVoigt::setParameter(size_t i, const double &value, bool explicitlySet) {
  API::IPeakFunction::setParameter(i, value, explicitlySet);

  g_log.debug() << "[PV] Set " << i << "-th parameter with value " << value << "\n";

  // explicitly set means that there is a chance that some parameter shall be
  // re-calculated
  // peak center shall be excluded as it does nothing to do with height, H,
  // intensity and mixing
  if (explicitlySet && i != 2) {
    g_log.debug() << "Update set history for " << i << "-th parameter"
                  << "\n";
    update_set_history(i);
    estimate_parameter_value();
  }

  return;
}

bool PseudoVoigt::estimate_parameter_value() {
  // peak center
  size_t to_calculate_index = get_parameter_to_calculate_from_set();
  g_log.debug() << "Time to calculate " << to_calculate_index << "-th parameter"
                << "\n";

  bool some_value_updated{true};
  if (to_calculate_index == 0) {
    // calculate mixings
    double peak_intensity = getParameter(1);
    double gamma = getParameter(3);
    double mixing = (m_height * 0.5 * M_PI * gamma / peak_intensity - 1) / (sqrt(M_PI * M_LN2) - 1);
    if (mixing < 0) {
      g_log.debug() << "Calculated mixing (eta) = " << mixing << " < 0.  Set to 0 instead"
                    << "\n";
      mixing = 0;
    } else if (mixing > 1) {
      g_log.debug() << "Calculated mixing (eta) = " << mixing << " > 1. Set to 1 instead"
                    << "\n";
      mixing = 1;
    }

    setParameter(0, mixing,
                 false); // no explicitly set: won't cause further re-calcualting
  } else if (to_calculate_index == 1) {
    // calculate intensity
    double eta = getParameter(0);
    double gamma = getParameter(3);
    double peakIntensity = m_height / 2. / (1 + (sqrt(M_PI * M_LN2) - 1) * eta) * (M_PI * gamma);
    setParameter(1, peakIntensity, false);
    g_log.debug() << "Estimate peak intensity = " << peakIntensity << "\n";
  } else if (to_calculate_index == 3) {
    // calculate peak width
    double eta = getParameter(0);
    double peakIntensity = getParameter(1);

    g_log.debug() << "Intensity = " << peakIntensity << ", height = " << m_height << ", mixing = " << eta << "\n";
    double gamma = peakIntensity * 2 * (1 + (sqrt(M_PI * M_LN2) - 1) * eta) / (M_PI * m_height);
    if (gamma < 1.E-10) {
      g_log.debug() << "Peak width (H or Gamma) = " << gamma << ". Set 1.E-2 instead"
                    << "\n";
    }
    setParameter(3, gamma, false);
  } else {
    // no value updated
    some_value_updated = false;
  }

  return some_value_updated;
}

/** The purpose of this is to track which user-specified parameter is outdated
 * and to be calculated
 * @brief PseudoVoigt::update_set_history
 * @param set_index:: 0 mixing, 1 intensity, 2 height, 3 fwhm
 */
void PseudoVoigt::update_set_history(size_t set_index) {
  if (set_index > 3)
    throw std::runtime_error("Parameter set up history index out of boundary");

  size_t prev_distance = m_set_history_distances[set_index];
  for (size_t i = 0; i < 4; ++i) {
    // no operation on any never-set or out-dated index
    if (m_set_history_distances[i] >= 3)
      continue;

    // only increase the historoy distance by 1 in case
    // the other parameters with those with distance less than the previous one
    // i.e., out of date or never been set
    if (m_set_history_distances[i] < prev_distance) {
      m_set_history_distances[i] += 1;
      // if out of range, don't worry about it
    }
  }

  // set the current one to 0 (most recent)
  m_set_history_distances[set_index] = 0;
}

/** As a parameter (height, width, intensity and mixing) being set (via
 * setParameter)
 * find out whethere are are enough parameters that have been specified
 * @brief PseudoVoigt::get_parameter_to_calculate_from_set
 * @return :: index of the parameter to calculate if there is
 */
size_t PseudoVoigt::get_parameter_to_calculate_from_set() {
  // go through setting-history-distance
  size_t index_largest_distance{0};
  size_t largest_distance{0};
  size_t num_been_set{0};

  for (size_t i = 0; i < m_set_history_distances.size(); ++i) {
    g_log.debug() << "distance[" << i << "] = " << m_set_history_distances[i] << "\n";
    if (m_set_history_distances[i] >= largest_distance) {
      // for those to set for: any number counts
      index_largest_distance = i;
      largest_distance = m_set_history_distances[i];
    }
    // only those been set and not out of date counted
    if (m_set_history_distances[i] < 3) {
      ++num_been_set;
    }
  }

  // signal for no enough index been set up
  if (num_been_set < 3)
    index_largest_distance = 128;

  return index_largest_distance;
}

/** calculate effective parameter: height
 * @return
 */
double PseudoVoigt::height() const {
  double peak_intensity = getParameter("Intensity");
  double gamma = getParameter("FWHM");
  double eta = getParameter("Mixing");
  const double height = 2. * peak_intensity * (1 + (sqrt(M_PI * M_LN2) - 1) * eta) / (M_PI * gamma);

  return height;
}

/** set non-native parameter peak height
 * @brief PseudoVoigt::setHeight
 * @param h
 */
void PseudoVoigt::setHeight(const double h) {
  // set height
  double prev_height = m_height;
  m_height = h;

  // update set history
  update_set_history(2);
  g_log.debug() << "PV: set height = " << m_height << "\n";

  // update other parameters' value
  bool updated_any_value = estimate_parameter_value();
  if (!updated_any_value) {
    // force to update peak intensity
    double old_intensity = getParameter("Intensity");
    // This is what Roman suggests!
    double new_intensity{1.};
    if (prev_height > 0)
      new_intensity = old_intensity * m_height / prev_height;
    else
      new_intensity = m_height; // this is a better than nothing estimation
    setParameter("Intensity", new_intensity,
                 false); // no need to update other parameters
  }
}

/** set FWHM
 * @param w
 */
void PseudoVoigt::setFwhm(const double w) { setParameter("FWHM", w, true); }

} // namespace Mantid::CurveFitting::Functions
