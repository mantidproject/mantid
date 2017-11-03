//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Lorentzian.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(Lorentzian)

Lorentzian::Lorentzian() : m_amplitudeEqualHeight(false) {}

void Lorentzian::init() {
  declareParameter("Amplitude", 1.0, "Intensity scaling");
  declareParameter("PeakCentre", 0.0, "Centre of peak");
  declareParameter("FWHM", 0.0, "Full-width at half-maximum");
}

double Lorentzian::height() const {
  double gamma = getParameter("FWHM");
  if (gamma == 0.0) {
    return getParameter("Amplitude");
  } else {
    return getParameter("Amplitude") * 2.0 / (gamma * M_PI);
  }
}

void Lorentzian::setHeight(const double h) {
  double gamma = getParameter("FWHM");
  if (gamma == 0.0) {
    m_amplitudeEqualHeight = true;
    setParameter("Amplitude", h);
  } else {
    setParameter("Amplitude", h * gamma * M_PI / 2.0);
  }
}

void Lorentzian::setFwhm(const double w) {
  auto gamma = getParameter("FWHM");
  if (gamma == 0.0 && w != 0.0 && m_amplitudeEqualHeight) {
    auto h = getParameter("Amplitude");
    setParameter("Amplitude", h * w * M_PI / 2.0);
  }
  if (w != 0.0) {
    m_amplitudeEqualHeight = false;
  }
  setParameter("FWHM", w);
}

void Lorentzian::fixCentre(bool isDefault) {
  fixParameter("PeakCentre", isDefault);
}

void Lorentzian::unfixCentre() { unfixParameter("PeakCentre"); }

void Lorentzian::fixIntensity(bool isDefault) {
  fixParameter("Amplitude", isDefault);
}

void Lorentzian::unfixIntensity() { unfixParameter("Amplitude"); }

void Lorentzian::functionLocal(double *out, const double *xValues,
                               const size_t nData) const {
  const double amplitude = getParameter("Amplitude");
  const double peakCentre = getParameter("PeakCentre");
  const double halfGamma = 0.5 * getParameter("FWHM");

  const double invPI = 1.0 / M_PI;
  for (size_t i = 0; i < nData; i++) {
    double diff = (xValues[i] - peakCentre);
    out[i] =
        amplitude * invPI * halfGamma / (diff * diff + (halfGamma * halfGamma));
  }
}

void Lorentzian::functionDerivLocal(Jacobian *out, const double *xValues,
                                    const size_t nData) {
  const double amplitude = getParameter("Amplitude");
  const double peakCentre = getParameter("PeakCentre");
  const double gamma = getParameter("FWHM");
  const double halfGamma = 0.5 * gamma;

  const double invPI = 1.0 / M_PI;
  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - peakCentre;
    const double invDen1 = 1.0 / (gamma * gamma + 4.0 * diff * diff);
    const double dfda = 2.0 * invPI * gamma * invDen1;
    out->set(i, 0, dfda);

    double invDen2 = 1 / (diff * diff + halfGamma * halfGamma);
    const double dfdxo = amplitude * invPI * gamma * diff * invDen2 * invDen2;
    out->set(i, 1, dfdxo);

    const double dfdg = -2.0 * amplitude * invPI *
                        (gamma * gamma - 4.0 * diff * diff) * invDen1 * invDen1;
    out->set(i, 2, dfdg);
  }
}

/// Calculate histogram data for the given bin boundaries.
/// @param out :: Output bin values (size == nBins) - integrals of the function
///    inside each bin.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void Lorentzian::histogram1D(double *out, double left, const double *right,
                             const size_t nBins) const {

  const double amplitude = getParameter("Amplitude");
  const double peakCentre = getParameter("PeakCentre");
  const double gamma = getParameter("FWHM");
  const double halfGamma = 0.5 * gamma;

  auto cumulFun = [halfGamma, peakCentre](double x) {
    return atan((x - peakCentre) / halfGamma) / M_PI;
  };
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
void Lorentzian::histogramDerivative1D(Jacobian *jacobian, double left,
                                       const double *right,
                                       const size_t nBins) const {
  const double amplitude = getParameter("Amplitude");
  const double c = getParameter("PeakCentre");
  const double g = getParameter("FWHM");
  const double g2 = g * g;

  auto cumulFun = [g, c](double x) { return atan((x - c) / g * 2) / M_PI; };
  auto denom = [g2, c](double x) { return (g2 + 4 * pow(c - x, 2)) * M_PI; };

  double xl = left;
  double denomLeft = denom(left);
  double cLeft = cumulFun(left);
  for (size_t i = 0; i < nBins; ++i) {
    double xr = right[i];
    double denomRight = denom(xr);
    double cRight = cumulFun(xr);
    jacobian->set(i, 0, cRight - cLeft);
    jacobian->set(i, 1, -2.0 * (g / denomRight - g / denomLeft) * amplitude);
    jacobian->set(i, 2, -2.0 * ((xr - c) / denomRight - (xl - c) / denomLeft) *
                            amplitude);
    denomLeft = denomRight;
    cLeft = cRight;
    xl = xr;
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
