#include "MantidCurveFitting/PseudoVoigt.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/BoundaryConstraint.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;

DECLARE_FUNCTION(PseudoVoigt);

void PseudoVoigt::functionLocal(double *out, const double *xValues,
                                const size_t nData) const {
  double h = getParameter("Height");
  double x0 = getParameter("PeakCentre");
  double f = getParameter("FWHM");

  double gFraction = getParameter("Mixing");
  double lFraction = 1.0 - gFraction;

  // Lorentzian parameter gamma...fwhm/2
  double g = f / 2.0;
  double gSquared = g * g;

  // Gaussian parameter sigma...fwhm/(2*sqrt(2*ln(2)))...gamma/sqrt(2*ln(2))
  double sSquared = gSquared / (2.0 * log(2.0));

  for (size_t i = 0; i < nData; ++i) {
    double xDiffSquared = (xValues[i] - x0) * (xValues[i] - x0);

    out[i] = h * (gFraction * exp(-0.5 * xDiffSquared / sSquared) +
             (lFraction * gSquared / (xDiffSquared + gSquared)));
  }
}

void PseudoVoigt::functionDerivLocal(Jacobian *out, const double *xValues,
                                     const size_t nData) {

  double h = getParameter("Height");
  double x0 = getParameter("PeakCentre");
  double f = getParameter("FWHM");

  double gFraction = getParameter("Mixing");
  double lFraction = 1.0 - gFraction;

  // Lorentzian parameter gamma...fwhm/2
  double g = f / 2.0;
  double gSquared = g * g;

  // Gaussian parameter sigma...fwhm/(2*sqrt(2*ln(2)))...gamma/sqrt(2*ln(2))
  double sSquared = gSquared / (2.0 * log(2.0));

  for (size_t i = 0; i < nData; ++i) {
    double xDiff = (xValues[i] - x0);
    double xDiffSquared = xDiff * xDiff;

    double expTerm = exp(-0.5 * xDiffSquared / sSquared);
    double lorentzTerm = gSquared / (xDiffSquared + gSquared);

    out->set(i, 0, h * (expTerm - lorentzTerm));
    out->set(i, 1, gFraction * expTerm + lFraction * lorentzTerm);
    out->set(i, 2, h * (gFraction * expTerm * xDiff / sSquared +
                        lFraction * lorentzTerm * xDiff * 2.0 /
                            (xDiffSquared + gSquared)));
    out->set(i, 3,
             gFraction * h * expTerm * xDiffSquared / sSquared / f +
                 lFraction * h * (lorentzTerm / g -
                                  lorentzTerm * g / (xDiffSquared + gSquared)));
  }
}

void PseudoVoigt::init() {
  declareParameter("Mixing");
  declareParameter("Height");
  declareParameter("PeakCentre");
  declareParameter("FWHM");

  BoundaryConstraint *mixingConstraint =
      new BoundaryConstraint(this, "Mixing", 0.0, 1.0, true);

  addConstraint(mixingConstraint);
}

} // namespace CurveFitting
} // namespace Mantid
