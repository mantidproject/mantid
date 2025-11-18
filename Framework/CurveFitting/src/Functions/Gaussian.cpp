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
  const auto peakHeight = getParameter("Height");
  const auto peakCentre = getParameter("PeakCentre");
  const auto weight = 1 / getParameter("Sigma");

  for (size_t i = 0; i < nData; i++) {
    const auto diff = (xValues[i] - peakCentre) * weight;
    out[i] = peakHeight * exp(-0.5 * diff * diff);
  }
}

void Gaussian::functionDerivLocal(Jacobian *out, const double *xValues, const size_t nData) {
  const auto peakHeight = getParameter("Height");
  const auto peakCentre = getParameter("PeakCentre");
  const auto weight = 1 / getParameter("Sigma");
  const auto weight_sq = weight * weight;

  for (size_t i = 0; i < nData; i++) {
    const auto diff = xValues[i] - peakCentre;
    const auto e = exp(-0.5 * diff * diff * weight_sq);
    out->set(i, 0, e);
    out->set(i, 1, diff * peakHeight * e * weight_sq);
    out->set(i, 2,
             -weight * diff * diff * peakHeight * e); // derivative with respect to weight not sigma
  }
}

void Gaussian::setActiveParameter(size_t i, double value) {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma")
    setParameter(i, 1. / value, false);
  else
    setParameter(i, value, false);
}

double Gaussian::activeParameter(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma")
    return 1. / getParameter(i);
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

  const auto amplitude = intensity();
  const auto peakCentre = getParameter("PeakCentre");
  const auto sigma2 = getParameter("Sigma") * sqrt(2.0);

  auto cumulFun = [sigma2, peakCentre](double x) { return 0.5 * erf((x - peakCentre) / sigma2); };
  auto cLeft = cumulFun(left);
  auto xLeft = left;
  for (size_t i = 0; i < nBins; ++i) {
    const auto bin_width = right[i] - xLeft;
    const auto cRight = cumulFun(right[i]);
    out[i] = amplitude * (cRight - cLeft) / bin_width;
    cLeft = cRight;
    xLeft = right[i];
  }
}

/// Derivatives of the histogram.
/// @param jacobian :: The output Jacobian.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void Gaussian::histogramDerivative1D(Jacobian *jacobian, double left, const double *right, const size_t nBins) const {
  const auto h = getParameter("Height");
  const auto c = getParameter("PeakCentre");
  const auto w = 1 / getParameter("Sigma");

  auto e = [w](const double d) { return exp(-0.5 * w * w * d * d); };
  auto eint = [w](double d) {
    return std::sqrt(M_PI / 2.0) * (1.0 / w) * std::erf(w * d / M_SQRT2);
  }; // integral of gaussian (h=1)

  auto dLeft = left - c;
  auto eLeft = e(dLeft);
  auto eintLeft = eint(dLeft);
  for (size_t i = 0; i < nBins; ++i) {
    const auto dRight = right[i] - c;
    const auto bin_width = dRight - dLeft;
    const auto eRight = e(dRight);
    const auto eintRight = eint(dRight);
    jacobian->set(i, 0, (eintRight - eintLeft) / bin_width); // height
    jacobian->set(i, 1, -h * (eRight - eLeft) / bin_width);  // centre
    jacobian->set(i, 2,
                  h * ((dRight * eRight - eintRight) - (dLeft * eLeft - eintLeft)) / (w * bin_width)); // weight
    eLeft = eRight;
    eintLeft = eintRight;
    dLeft = dRight;
  }
}

} // namespace Mantid::CurveFitting::Functions
