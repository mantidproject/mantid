//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ApodizationFunctionHelper.h"
#include <cmath>

/**
 * Returns the evaluation of the Bartlett
 * apodization function at a time (t) and
 * decay constant tau:
 * f = 1 - abs(t)/tau
 * @param time :: [input] current time (t)
 * @param decayConstant :: [input] the decay constant (tau)
 * @returns :: Function evaluation
 */
double bartlett(const double time, const double decayConstant) {
  return (1. - (std::abs(time) / decayConstant));
}
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
* Returns the evaluation of the Connes
* apodization function at a time (t) and
* decay constant tau:
* f = (1 - t^2/tau^2)^2
* @param time :: [input] current time (t)
* @param decayConstant :: [input] the decay constant (tau)
* @returns :: Function evaluation
*/
double connes(const double time, const double decayConstant) {
  double tmp = 1. - (time * time / (decayConstant * decayConstant));
  return tmp * tmp;
}
/**
* Returns the evaluation of the cosine
* apodization function at a time (t) and
* decay constant tau:
* f = cos(pi*t/(2.*tau))
* @param time :: [input] current time (t)
* @param decayConstant :: [input] the decay constant (tau)
* @returns :: Function evaluation
*/
double cosine(const double time, const double decayConstant) {
  return cos(time * M_PI / (2. * decayConstant));
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
* Returns the evaluation of the Welch
* apodization function at a time (t) and
* decay constant tau:
* f = 1 - t^2/tau^2
* @param time :: [input] current time (t)
* @param decayConstant :: [input] the decay constant (tau)
* @returns :: Function evaluation
*/
double welch(const double time, const double decayConstant) {
  return (1. - (time * time) / (decayConstant * decayConstant));
}
/**
* Returns no
* apodization function. i.e. the data is unchanged
* @param time :: [input] current time (t)
* @param decayConstant :: [input] the decay constant (tau)
* @returns :: Function evaluation
*/
double none(const double, const double) { return 1.; }
