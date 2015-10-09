//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Lorentzian.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Lorentzian)

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
    setParameter("Amplitude", h);
  } else {
    setParameter("Amplitude", h * gamma * M_PI / 2.0);
  }
}

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

} // namespace CurveFitting
} // namespace Mantid
