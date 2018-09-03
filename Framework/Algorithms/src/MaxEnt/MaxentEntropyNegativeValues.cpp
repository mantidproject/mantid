#include "MantidAlgorithms/MaxEnt/MaxentEntropyNegativeValues.h"
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
MaxentEntropyNegativeValues::derivative(const std::vector<double> &values,
                                        double background) {

  std::vector<double> result(values.size());

  // First derivative
  for (size_t i = 0; i < values.size(); i++) {
    double normVal = values[i] / background;
    result[i] = -std::log(normVal + std::sqrt(normVal * normVal + 1));
  }
  return result;
}

/**
 * Returns the second derivative at a given point.
 * @param values : [input] The values of the image as a vector
 * @param background : [input] The background
 * @return : The second derivative as a vector
 */
std::vector<double>
MaxentEntropyNegativeValues::secondDerivative(const std::vector<double> &values,
                                              double background) {

  std::vector<double> result(values.size());

  double bkg2 = background * background;

  // Second derivative
  for (size_t i = 0; i < values.size(); i++) {
    result[i] = std::sqrt(values[i] * values[i] + bkg2);
  }
  return result;
}

/**
 * Corrects the image. For PosNeg images there is nothing to correct so we just
 * return the vector we were given
 * @param values : [input] The values of the image as a vector
 * @param newValue : [input] The new value to use (unused variable)
 * @return : The corrected values as a vector
 */
std::vector<double>
MaxentEntropyNegativeValues::correctValues(const std::vector<double> &values,
                                           double newValue) {

  UNUSED_ARG(newValue)

  // Nothing to correct
  return values;
}

} // namespace Algorithms
} // namespace Mantid
