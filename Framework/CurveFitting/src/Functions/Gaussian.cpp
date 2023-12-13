// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>
#include <numeric>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Gaussian)

Gaussian::Gaussian() : IPeakFunction(), m_intensityCache(0.0) {}

void Gaussian::init() {
  declareParameter("Height", 0.0, "Height of peak");
  declareParameter("PeakCentre", 0.0, "Centre of peak");
  declareParameter("Sigma", 0.0, "Width parameter");
}

void Gaussian::functionLocal(double *out, const double *xValues, const size_t nData) const {
  const double peakHeight = getParameter("Height");
  const double peakCentre = getParameter("PeakCentre");
  const double weight = pow(1 / getParameter("Sigma"), 2);

  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - peakCentre;
    out[i] = peakHeight * exp(-0.5 * diff * diff * weight);
  }
}

void Gaussian::functionDerivLocal(Jacobian *out, const double *xValues, const size_t nData) {
  const double peakHeight = getParameter("Height");
  const double peakCentre = getParameter("PeakCentre");
  const double weight = pow(1 / getParameter("Sigma"), 2);

  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - peakCentre;
    double e = exp(-0.5 * diff * diff * weight);
    out->set(i, 0, e);
    out->set(i, 1, diff * peakHeight * e * weight);
    out->set(i, 2,
             -0.5 * diff * diff * peakHeight * e); // derivative with respect to weight not sigma
  }
}

void Gaussian::setActiveParameter(size_t i, double value) {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma")
    setParameter(i, sqrt(fabs(1. / value)), false);
  else
    setParameter(i, value, false);
}

double Gaussian::activeParameter(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma")
    return 1. / pow(getParameter(i), 2);
  else
    return getParameter(i);
}

double Gaussian::centre() const { return getParameter("PeakCentre"); }
double Gaussian::height() const { return getParameter("Height"); }
double Gaussian::fwhm() const { return 2.0 * sqrt(2.0 * M_LN2) * getParameter("Sigma"); }
double Gaussian::intensity() const {
  auto sigma = getParameter("Sigma");
  if (sigma == 0.0) {
    auto peakHeight = getParameter("Height");
    if (std::isfinite(peakHeight)) {
      m_intensityCache = peakHeight;
    }
  } else {
    m_intensityCache = getParameter("Height") * getParameter("Sigma") * sqrt(2.0 * M_PI);
  }
  return m_intensityCache;
}
double Gaussian::intensityError() const {
  const double heightError = getError("Height");
  const double sigmaError = getError("Sigma");

  return intensity() * sqrt(pow(heightError / getParameter("Height"), 2) + pow(sigmaError / getParameter("Sigma"), 2));
}

void Gaussian::setCentre(const double c) { setParameter("PeakCentre", c); }
void Gaussian::setHeight(const double h) { setParameter("Height", h); }
void Gaussian::setFwhm(const double w) { setParameter("Sigma", w / (2.0 * sqrt(2.0 * M_LN2))); }
void Gaussian::setIntensity(const double i) {
  m_intensityCache = i;
  auto sigma = getParameter("Sigma");
  if (sigma == 0.0) {
    setParameter("Height", i);
  } else {
    setParameter("Height", i / (sigma * sqrt(2.0 * M_PI)));
  }
}

void Gaussian::fixCentre(bool isDefault) { fixParameter("PeakCentre", isDefault); }

void Gaussian::unfixCentre() { unfixParameter("PeakCentre"); }

void Gaussian::fixIntensity(bool isDefault) {
  std::string formula = std::to_string(intensity() / sqrt(2.0 * M_PI)) + "/Sigma";
  tie("Height", formula, isDefault);
}

void Gaussian::unfixIntensity() { removeTie("Height"); }

/// Calculate histogram data for the given bin boundaries.
/// @param out :: Output bin values (size == nBins) - integrals of the function
///    inside each bin.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void Gaussian::histogram1D(double *out, double left, const double *right, const size_t nBins) const {

  double amplitude = intensity();
  const double peakCentre = getParameter("PeakCentre");
  const double sigma2 = getParameter("Sigma") * sqrt(2.0);

  auto cumulFun = [sigma2, peakCentre](double x) { return 0.5 * erf((x - peakCentre) / sigma2); };
  double cLeft = cumulFun(left);
  for (size_t i = 0; i < nBins; ++i) {
    double cRight = cumulFun(right[i]);
    out[i] = amplitude * (cRight - cLeft);
    cLeft = cRight;
  }
}

/// Derivatives of the histogram.
/// @param jacobian :: The output Jacobian.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void Gaussian::histogramDerivative1D(Jacobian *jacobian, double left, const double *right, const size_t nBins) const {
  const double h = getParameter("Height");
  const double c = getParameter("PeakCentre");
  const double s = getParameter("Sigma");
  const double w = pow(1 / s, 2);
  const double sw = sqrt(w);

  auto cumulFun = [sw, c](double x) { return sqrt(M_PI / 2) / sw * erf(sw / sqrt(2.0) * (x - c)); };
  auto fun = [w, c](double x) { return exp(-w / 2 * pow(x - c, 2)); };

  double xl = left;
  double fLeft = fun(left);
  double cLeft = cumulFun(left);
  const double h_over_2w = h / (2 * w);
  for (size_t i = 0; i < nBins; ++i) {
    double xr = right[i];
    double fRight = fun(xr);
    double cRight = cumulFun(xr);
    jacobian->set(i, 0, cRight - cLeft);        // height
    jacobian->set(i, 1, -h * (fRight - fLeft)); // centre
    jacobian->set(i, 2,
                  h_over_2w * ((xr - c) * fRight - (xl - c) * fLeft + cLeft - cRight)); // weight
    fLeft = fRight;
    cLeft = cRight;
    xl = xr;
  }
}

} // namespace Mantid::CurveFitting::Functions
