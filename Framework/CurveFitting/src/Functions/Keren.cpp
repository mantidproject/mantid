#include "MantidCurveFitting/Functions/Keren.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>

namespace {
constexpr double TWOPI = 2.0 * M_PI;
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(Keren)

/**
 * Initialise parameters for function
 */
void Keren::init() {
  declareParameter("A", 1.0, "Polarization at time zero");
  declareParameter("Delta", 0.2, "Distribution width of local fields (MHz)");
  declareParameter("Field", 50.0, "Longitudinal field (Gauss)");
  declareParameter("Fluct", 0.2,
                   "Hopping rate (inverse correlation time, MHz)");
}

/**
 * Set the ith active parameter's value
 * We use the Larmor frequency rather than the field
 * omega_L = 2 * pi * gamma_mu * B
 * @param i :: [input] Index of parameter
 * @param value :: [input, output] Value of ith parameter
 * @throws std::runtime_error if param is inactive (fixed)
 */
void Keren::setActiveParameter(size_t i, double value) {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Field") {
    // Value passed in is omega_L, return B
    setParameter(i, value / (PhysicalConstants::MuonGyromagneticRatio * TWOPI),
                 false);
  } else
    setParameter(i, value, false);
}

/**
 * Get the ith active parameter's value
 * We use the Larmor frequency rather than the field
 * omega_L = 2 * pi * gamma_mu * B
 * @param i :: [input] Index of parameter
 * @returns :: Value of ith parameter
 * @throws std::runtime_error if param is inactive (fixed)
 */
double Keren::activeParameter(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Field") {
    // Fit to omega_L = 2 * pi * gamma_mu * B
    return getParameter(i) * PhysicalConstants::MuonGyromagneticRatio * TWOPI;
  } else
    return getParameter(i);
}

/**
 * Calculate the value of the function for each given X value
 * @param out :: [output] Array to be filled with function values
 * @param xValues :: [input] Array of X values to calculate function at
 * @param nData :: [input] Number of X values
 */
void Keren::function1D(double *out, const double *xValues,
                       const size_t nData) const {
  // Get parameters
  const double initial = getParameter("A");
  const double delta = getParameter("Delta");
  const double fluct = getParameter("Fluct");
  const double larmor =
      getParameter("Field") * PhysicalConstants::MuonGyromagneticRatio * TWOPI;

  for (size_t i = 0; i < nData; i++) {
    out[i] = initial * polarization(delta, larmor, fluct, xValues[i]);
  }
}

/**
 * Calculate the muon polarization P_z(t) at time t as a fraction of the initial
 * polarization (P_0(t) = 1)
 * Equation (3) in the paper
 * @param delta :: [input] Delta, distribution width of local fields in MHz
 * @param larmor :: [input] omega_L, Larmor frequency in MHz
 * @param fluct :: [input] nu, hopping rate in MHz
 * @param time :: [input] t, time in microseconds
 * @returns :: Polarization P_z(t) (dimensionless)
 */
double Keren::polarization(const double delta, const double larmor,
                           const double fluct, const double time) const {
  return exp(-1.0 * relaxation(delta, larmor, fluct, time));
}

/**
 * Calculate the relaxation Gamma(t)t at time t
 * Equation (4) in the paper
 * @param delta :: [input] Delta, distribution width of local fields in MHz
 * @param larmor :: [input] omega_L, Larmor frequency in MHz
 * @param fluct :: [input] nu, hopping rate in MHz
 * @param time :: [input] t, time in microseconds
 * @returns :: Relaxation Gamma(t)*t (dimensionless)
 */
double Keren::relaxation(const double delta, const double larmor,
                         const double fluct, const double time) const {
  // Useful shortcuts
  const double deltaSq = delta * delta;
  const double omegaSq = larmor * larmor;
  const double nuSq = fluct * fluct;
  const double omegaT = larmor * time;
  const double nuT = fluct * time;
  const double expon = exp(-1.0 * nuT);

  // 2*Delta^2 / (omega_L^2 + nu^2)^2
  const double prefactor =
      (2.0 * deltaSq) / ((omegaSq + nuSq) * (omegaSq + nuSq));

  const double term1 = (omegaSq + nuSq) * nuT;
  const double term2 = omegaSq - nuSq;
  const double term3 = 1.0 - expon * cos(omegaT);
  const double term4 = 2.0 * fluct * larmor * expon * sin(omegaT);

  return prefactor * (term1 + term2 * term3 - term4);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
