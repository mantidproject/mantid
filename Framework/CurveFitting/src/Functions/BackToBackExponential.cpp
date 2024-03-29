// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BackToBackExponential.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_sf_lambert.h>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(BackToBackExponential)

void BackToBackExponential::init() {
  // Do not change the order of these parameters!
  declareParameter("I", 0.0, "integrated intensity of the peak"); // 0
  declareParameter("A", 1.0,
                   "exponential constant of rising part of neutron pulse");              // 1
  declareParameter("B", 0.05, "exponential constant of decaying part of neutron pulse"); // 2
  declareParameter("X0", 0.0, "peak position");                                          // 3
  declareParameter("S", 1.0,
                   "standard deviation of gaussian part of peakshape function"); // 4
}

/**
 * Get approximate height (maximum value) of the peak (not at X0)
 */
double BackToBackExponential::height() const {
  // height = I * ln2 /(FWHM - ln2 * S)
  const double I = getParameter(0);
  const double s = getParameter(4);
  const auto width = fwhm();
  const auto h = I * M_LN2 / (width - M_LN2 * s);
  return h;
}

/**
 * Set new height of the peak. This method does this approximately.
 * @param h :: New value for the height.
 */
void BackToBackExponential::setHeight(const double h) {
  double h0 = height();
  if (h0 == 0.0) {
    setParameter(0, 1e-6);
    h0 = height();
  }
  double area = getParameter(0); // ==I
  area *= h / h0;
  if (area <= 0.0) {
    area = 1e-6;
  }
  if (!std::isfinite(area)) {
    area = std::numeric_limits<double>::max() / 2;
  }
  setParameter(0, area);
}

/**
 * Get approximate peak width.
 */
double BackToBackExponential::fwhm() const {
  // intrinsic width of B2B exp
  auto s = getParameter("S");
  auto w0 = expWidth();
  // Gaussian and B2B exp widths don't add in quadrature. The following
  // tends to gaussian at large S and at S=0 is equal to the intrinsic width of
  // the B2B exp (good to <3% for typical params)
  return w0 * exp(-0.5 * M_LN2 * s / w0) + 2 * sqrt(2 * M_LN2) * s;
}

/**
 * Set new peak width approximately.
 * @param w :: New value for the width.
 */
void BackToBackExponential::setFwhm(const double w) {
  // get height so can reset after changing S
  const auto h0 = height();
  const auto w0 = expWidth();
  if (w > w0) {
    const auto a = 0.5 * M_LN2;
    const auto b = 2 * sqrt(2 * M_LN2);
    // calculate new value of S (from solving eq in fwhm func)
    setParameter("S", w0 * (gsl_sf_lambert_W0(-(a / b) * exp(-(a / b) * (w / w0))) / a + (w / w0) / b));
  } else {
    // set to some small number relative to w0
    setParameter("S", 1e-6);
  }
  // reset height
  setHeight(h0);
}

void BackToBackExponential::function1D(double *out, const double *xValues, const size_t nData) const {
  /*
    const double& I = getParameter("I");
    const double& a = getParameter("A");
    const double& b = getParameter("B");
    const double& x0 = getParameter("X0");
    const double& s = getParameter("S");
  */

  const double I = getParameter(0);
  const double a = getParameter(1);
  const double b = getParameter(2);
  const double x0 = getParameter(3);
  const double s = getParameter(4);

  // find the reasonable extent of the peak ~100 fwhm
  double extent = expWidth();
  if (s > extent)
    extent = s;
  extent *= 100;

  double s2 = s * s;
  double normFactor = a * b / (a + b) / 2;
  // Needed for IntegratePeaksMD for cylinder profile fitted with b=0
  if (normFactor == 0.0)
    normFactor = 1.0;
  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - x0;
    if (fabs(diff) < extent) {
      double val = 0.0;
      double arg1 = a / 2 * (a * s2 + 2 * diff);
      val += exp(arg1 + gsl_sf_log_erfc((a * s2 + diff) / sqrt(2 * s2))); // prevent overflow
      double arg2 = b / 2 * (b * s2 - 2 * diff);
      val += exp(arg2 + gsl_sf_log_erfc((b * s2 - diff) / sqrt(2 * s2))); // prevent overflow
      out[i] = I * val * normFactor;
    } else
      out[i] = 0.0;
  }
}

/**
 * Evaluate function derivatives numerically.
 */
void BackToBackExponential::functionDeriv1D(Jacobian *jacobian, const double *xValues, const size_t nData) {
  FunctionDomain1DView domain(xValues, nData);
  this->calNumericalDeriv(domain, *jacobian);
}

/**
 * Calculate contribution to the width by the exponentials.
 */
double BackToBackExponential::expWidth() const {
  const double a = getParameter(1);
  const double b = getParameter(2);
  // Needed for IntegratePeaksMD for cylinder profile fitted with b=0
  if (a * b == 0.0)
    return M_LN2;
  return M_LN2 * (a + b) / (a * b);
}

} // namespace Mantid::CurveFitting::Functions
