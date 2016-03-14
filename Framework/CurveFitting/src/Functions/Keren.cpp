#include "MantidCurveFitting/Functions/Keren.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(Keren)

/**
 * Initialise parameters for function
 */
void Keren::init() {
  declareParameter("A", 1.0, "Polarization at time zero");
  declareParameter("Delta", 0.2, "Distribution width of local field");
  declareParameter("Field", 0.2, "Longitudinal field");
  declareParameter("Fluct", 0.2, "Hopping rate (inverse correlation time)");
}

/**
 * Set the ith active parameter's value
 * We use the Larmor frequency rather than the field
 * omega_L = gamma_mu * B
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
    setParameter(i, value / PhysicalConstants::MuonGyromagneticRatio, false);
  } else
    setParameter(i, value, false);
}

/**
 * Get the ith active parameter's value
 * We use the Larmor frequency rather than the field
 * omega_L = gamma_mu * B
 * @param i :: [input] Index of parameter
 * @returns :: Value of ith parameter
 * @throws std::runtime_error if param is inactive (fixed)
 */
double Keren::activeParameter(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Field") {
    // Fit to omega_L = gamma_mu * B
    return getParameter(i) * PhysicalConstants::MuonGyromagneticRatio;
  } else
    return getParameter(i);
}

void Keren::function1D(double *out, const double *xValues,
                       const size_t nData) const {}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
