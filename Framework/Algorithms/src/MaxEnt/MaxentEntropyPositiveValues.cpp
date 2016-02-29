#include "MantidAlgorithms/MaxEnt/MaxentEntropyPositiveValues.h"

namespace Mantid {
namespace Algorithms {

/**
* Returns the first derivative at a given point.
* @param value : [input] The value of the image at a specific point divided by a
* background
* @return : The first derivative
*/
double MaxentEntropyPositiveValues::getDerivative(double value) {

  return (-log(value));
}

/**
* Returns the second derivative at a given point.
* @param value : [input] The value of the image at a specific point divided by a
* background
* @return : The second derivative
*/
double MaxentEntropyPositiveValues::getSecondDerivative(double value) {

  return value;
}

/**
* Corrects a negative value
* @param value : [input] The value of the image at a specific point
* @param newValue : [input] The new value to use
* @return : The corrected value
*/
double MaxentEntropyPositiveValues::correctValue(double value,
                                                 double newValue) {

  if (value < 0)
    value = newValue;

  return value;
}

} // namespace Algorithms
} // namespace Mantid
