#include "MantidAlgorithms/MaxEnt/MaxentEntropyPositiveValues.h"
#include <cmath>

namespace Mantid {
namespace Algorithms {

/**
 * Returns the first derivative at a given point.
 * @param values : [input] The values of the image as a vector
 * @param background : [input] The background
 * @return : The first derivative as a vector
 */
std::vector<double>
MaxentEntropyPositiveValues::derivative(const std::vector<double> &values,
                                        double background) {

  std::vector<double> result(values.size());

  // First derivative
  for (size_t i = 0; i < values.size(); i++) {
    result[i] = -std::log(values[i] / background);
  }
  return result;
}

/**
 * Returns the second derivative at a given point.
 * @param values : [input] The values of the image as a vector
 * @param background : [input] The background (unused)
 * @return : The second derivative as a vector
 */
std::vector<double>
MaxentEntropyPositiveValues::secondDerivative(const std::vector<double> &values,
                                              double background) {

  UNUSED_ARG(background);
  // This is referred to as 'second derivative' in the paper, but in the codes
  // I've seen is just the input vector
  return values;
}

/**
 * Corrects a negative value
 * @param values : [input] The values of the image as a vector
 * @param newValue : [input] The new value to use
 * @return : The corrected values
 */
std::vector<double>
MaxentEntropyPositiveValues::correctValues(const std::vector<double> &values,
                                           double newValue) {

  std::vector<double> result(values.size());

  for (size_t i = 0; i < values.size(); i++) {
    if (values[i] < 0)
      result[i] = newValue;
    else
      result[i] = values[i];
  }

  return result;
}

} // namespace Algorithms
} // namespace Mantid
