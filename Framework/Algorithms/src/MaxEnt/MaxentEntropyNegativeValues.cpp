#include "MantidAlgorithms/MaxEnt/MaxentEntropyNegativeValues.h"

namespace Mantid {
namespace Algorithms {

/**
* Returns the first derivative at a given point.
* @param value : [input] The value of the image at a specific point divided by a
* background
* @return : The first derivative
*/
double MaxentEntropyNegativeValues::getDerivative(double value) {

  // First derivative
  return (-log(value + std::sqrt(value * value + 1)));
}

/**
* Returns the second derivative at a given point.
* @param value : [input] The value of the image at a specific point divided by a
* background
* @return : The second derivative
*/
double MaxentEntropyNegativeValues::getSecondDerivative(double value) {

  return fabs(value);
}

/**
* Corrects a value. For PosNeg images there is nothing to correct
* @param value : [input] The value of the image at a specific point
* @param newValue : [input] The new value to use (unused variable)
* @return : The corrected value
*/
double MaxentEntropyNegativeValues::correctValue(double value,
                                                 double newValue) {

  UNUSED_ARG(newValue)

  // Nothing to correct
  return value;
}

} // namespace Algorithms
} // namespace Mantid
