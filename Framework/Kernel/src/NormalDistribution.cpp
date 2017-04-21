//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <string>
#include "MantidKernel/NormalDistribution.h"
#include "MantidKernel/MersenneTwister.h"

namespace Mantid {
namespace Kernel {

//------------------------------------------------------------------------------
// Public member functions
//------------------------------------------------------------------------------

/// Construct the default generator for standard normal distribution
/// (mean = 0.0, sigma = 1.0)
NormalDistribution::NormalDistribution()
    : m_uniform_generator(), m_generator(0.0, 1.0) {}

/// Construct the generator with initial distribution parameters
/// and default seed.
NormalDistribution::NormalDistribution(const double mean, const double sigma)
    : m_uniform_generator(), m_generator(mean, sigma) {
  if (sigma <= 0.0) {
    throw std::runtime_error(
        "Normal distribution must have positive sigma, given " +
        std::to_string(sigma));
  }
}

/// Construct the generator with initial distribution parameters and
/// a seed value.
NormalDistribution::NormalDistribution(const size_t seedValue,
                                       const double mean, const double sigma)
    : m_uniform_generator(seedValue), m_generator(mean, sigma) {
  if (sigma <= 0.0) {
    throw std::runtime_error(
        "Normal distribution must have positive sigma, given " +
        std::to_string(sigma));
  }
}

/// Set the random number seed.
/// @param seedValue :: A seed for the generator.
void NormalDistribution::setSeed(const size_t seedValue) {
  m_uniform_generator.setSeed(seedValue);
}

/// Generate the next random number in the sequence
double NormalDistribution::nextValue() {
  return m_generator(m_uniform_generator);
}

/// Generate a random number from a distribution with given mean and sigma.
/// @param argMean :: A mean of the distribution.
/// @param argSigma :: A sigma of the distribution.
double NormalDistribution::randomValue(double argMean, double argSigma) {
  boost::normal_distribution<double>::param_type param(argMean, argSigma);
  return m_generator(m_uniform_generator, param);
}
}
}
