#include "MantidKernel/Math/Distributions/BoseEinsteinDistribution.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Mantid {
namespace Kernel {
namespace Math {
namespace {
/// Tolerance to consider value zero
double ZERO_EPS = 1e-12;
/// Forward declaration of helper to raise domain error
void throwDomainError(const std::string &msg, const double value);
/// Forward declaration of helper to calculate distribution
double yOver1MinusExpY(const double);
}

/**
 * Calculate the expected number of particles in an energy state at a given
 * temperature
 * for a Bose-Einstein distribution: \f$1/[exp(\epsilon/k_BT) - 1]\f$. Note this
 * function
 * is not well behaved for y=0. @see np1Eps
 * @param energy :: The value of the energy of the state in meV
 * @param temperature :: The temperature in Kelvin
 * @return The value of the distribution
 */
double BoseEinsteinDistribution::n(const double energy,
                                   const double temperature) {
  const double kbT = PhysicalConstants::BoltzmannConstant * temperature;
  if (std::abs(temperature) < ZERO_EPS)
    throwDomainError("BoseEinsteinDistribution::n - Temperature very small, "
                     "function not well behaved",
                     temperature);

  const double beta = energy / kbT;
  if (std::abs(beta) < ZERO_EPS)
    throwDomainError("BoseEinsteinDistribution::n - Exponent very small, "
                     "function not well-behaved",
                     beta);

  return 1.0 / (std::exp(beta) - 1.0);
}

/**
 * Calculate the expected number of particles in an energy state at a given
 * temperature
 * for a Bose-Einstein distribution: \f$1/[exp(\epsilon/k_BT) - 1]\f$. Deals
 * with edge cases where
 *    exponent = 0 using a taylor series
 *    large & negative exponent by multiplying by \f$exp(-y)\f$
 * @param energy :: The value of the energy of the state in meV
 * @param temperature :: The temperature in Kelvin
 * @return The value of the distribution
 */
double BoseEinsteinDistribution::np1Eps(const double energy,
                                        const double temperature) {
  if (temperature < ZERO_EPS) {
    if (energy < 0.0)
      return 0.0;
    else
      return energy;
  } else {
    const double kBT = (PhysicalConstants::BoltzmannConstant * temperature);
    return kBT * yOver1MinusExpY(energy / kBT);
  }
}

//---------------------------------------------------------------------
// Non-member functions
//---------------------------------------------------------------------
namespace {
/**
 * Raises a domain error with the given error message
 * @param msg :: Error message
 * @param value :: Appends string Value=value after
 */
void throwDomainError(const std::string &msg, const double value) {
  throw std::domain_error(msg + " Value=" +
                          boost::lexical_cast<std::string>(value));
}

/**
 * Calculate y/(1-exp(-y)) dealing with edges cases at
 *   y = 0 : using Taylor series
 *   large negative y: multiplying top & bottom by exp(-y)
 * @param y The value of y
 * @return The value of y/(1-exp(-y))
 */
double yOver1MinusExpY(const double y) {
  double magnitudeY = std::abs(y);
  if (magnitudeY > 0.1) {
    const double expMinusY = std::exp(-magnitudeY);
    double result = magnitudeY / (1.0 - expMinusY);
    if (y < 0)
      result *= expMinusY;
    return result;
  } else {
    // Taylor series coefficients
    const double by2(0.5), by6(1.0 / 6.0), by60(1.0 / 60.0), by42(1.0 / 42.0),
        by40(1.0 / 40.0);
    const double ysqr = y * y;
    return 1.0 +
           by2 * y *
               (1.0 +
                by6 * y *
                    (1.0 -
                     by60 * ysqr * (1.0 - by42 * ysqr * (1.0 - by40 * ysqr))));
  }
}
} // end anonyomous

} // end Math
}
} // end Mantid::Kernel
