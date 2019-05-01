// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ApodizationFunctions.h"
#include <cmath>

namespace Mantid {
namespace Algorithms {
namespace ApodizationFunctions {

/**
 * Returns the evaluation of the Lorentz
 * (an exponential decay)
 * apodization function at a time (t) and
 * decay constant tau:
 * f = exp(-t/tau)
 * @param time :: [input] current time (t)
 * @param decayConstant :: [input] the decay constant (tau)
 * @returns :: Function evaluation
 */
double lorentz(const double time, const double decayConstant) {
  return exp(-time / decayConstant);
}
/**
 * Returns the evaluation of the Gaussian
 * apodization function at a time (t) and
 * decay constant tau:
 * f =exp(-time^2/(2*tau^2))
 * @param time :: [input] current time (t)
 * @param decayConstant :: [input] the decay constant (tau)
 * @returns :: Function evaluation
 */
double gaussian(const double time, const double decayConstant) {
  return exp(-(time * time) / (2. * decayConstant * decayConstant));
}

/**
 * Returns no
 * apodization function. i.e. the data is unchanged
 * @param :: [input] current time (t)
 * @param :: [input] the decay constant (tau)
 * @returns :: Function evaluation
 */
double none(const double /*unused*/, const double /*unused*/) { return 1.; }
} // namespace ApodizationFunctions
} // namespace Algorithms
} // namespace Mantid
