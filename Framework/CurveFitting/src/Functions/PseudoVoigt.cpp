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

void PseudoVoigt::functionLocal(double *out, const double *xValues,
                                const size_t nData) const {
  const double intensity = getParameter("Intensity");
  const double x0 = getParameter("PeakCentre");
  const double gamma = fabs(getParameter("FWHM"));

  if (gamma < 1.E-20)
    throw std::runtime_error("Pseudo-voigt has FWHM as 0. It will generate "
                             "infinity at center in the Lorentzian part.");

  const double gFraction = getParameter("Mixing");
  const double lFraction = 1.0 - gFraction;

  const double ag = 2. / gamma * sqrt(M_LN2 / pi);
  // const double sigma = 0.5 / sqrt(2 * M_LN2) * gamma
  // const double sigmasq = sigma * sigma;
  const double gammasq = gamma * gamma;
  const double sigmasq = gammasq * 0.25 / (2. * M_LN2);

  for (size_t i = 0; i < nData; ++i) {
    double xDiffSquared = (xValues[i] - x0) * (xValues[i] - x0);

    out[i] = intensity * (gFraction * ag * exp(-0.5 * xDiffSquared / sigmasq) +
                          (lFraction * gamma / (xDiffSquared + gamma * gamma)));
  }
}

void PseudoVoigt::functionDerivLocal(Jacobian *out, const double *xValues,
                                     const size_t nData) {

  double intensity = getParameter("Intensity");
  double x0 = getParameter("PeakCentre");
  double f = getParameter("FWHM");

  double gFraction = getParameter("Mixing");
  double lFraction = 1.0 - gFraction;

  const double ag = 2. / gamma * sqrt(M_LN2 / pi);
  const double sigmasq = gammasq * 0.25 / (2. * M_LN2);

  // // Lorentzian parameter gamma...fwhm/2
  // double g = f / 2.0;
  // double gSquared = g * g;

  // Gaussian parameter sigma...fwhm/(2*sqrt(2*ln(2)))...gamma/sqrt(2*ln(2))
  // double sSquared = gSquared / (2.0 * M_LN2);

  for (size_t i = 0; i < nData; ++i) {
    double xDiff = (xValues[i] - x0);
    double xDiffSquared = xDiff * xDiff;

    double gaussian_term = ag * exp(-0.5 * xDiffSquared / sigmasq);
    double lorentzan_term = gamma / (xDiffSquared + gamma * gamma);

    // mixing
    out->set(i, 0, intensity * (gaussian_term - lorentzian_term));
    // derivative on intensity
    out->set(i, 1, gFraction * gaussian_term + lFraction * lorentzian_term);
    // peak center: x0
    // TODO FIXME this is not correct
    out->set(i, 2,
             h * xDiff *
                 (gFraction * expTerm / sSquared +
                  lFraction * lorentzTerm * 2.0 / (xDiffSquared + gSquared)));
    // peak width
    // TODO FIXME this is not correct
    out->set(i, 3,
             h * (gFraction * expTerm * xDiffSquared / sSquared / f +
                  lFraction * lorentzTerm *
                      (1.0 / g - g / (xDiffSquared + gSquared))));
  }
}

void PseudoVoigt::init() {
  declareParameter("Mixing", 1.0);
  declareParameter("Intensity");
  declareParameter("PeakCentre");
  declareParameter("FWHM");

  auto mixingConstraint =
      Kernel::make_unique<BoundaryConstraint>(this, "Mixing", 0.0, 1.0, true);
  mixingConstraint->setPenaltyFactor(1e9);

  // TODO - Add a constrain on FWHM too

  addConstraint(std::move(mixingConstraint));
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
