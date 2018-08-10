#include "MantidAlgorithms/MaxEnt/MaxentSpaceReal.h"
#include <stdexcept>

namespace Mantid {
namespace Algorithms {

/**
 * Converts a vector of real values to a vector of complex numbers
 * @param values : [input] The real values as a vector
 * @return : The input as a vector of complex numbers
 */
std::vector<double>
MaxentSpaceReal::toComplex(const std::vector<double> &values) {

  // The output has 2*N values
  std::vector<double> result(values.size() * 2);

  for (size_t i = 0; i < values.size(); i++)
    result[2 * i] = values[i];

  return result;
}

/**
 * Converts a vector of complex numbers to a vector of real numbers
 * @param values : [input] The complex values as a vector
 * @return : The input as a vector of real numbers
 */
std::vector<double>
MaxentSpaceReal::fromComplex(const std::vector<double> &values) {

  if (values.size() % 2) {
    throw std::invalid_argument("Cannot convert to real vector");
  }

  // The output has N/2 values
  std::vector<double> result(values.size() / 2);

  for (size_t i = 0; i < result.size(); i++) {
    result[i] = values[2 * i];
  }
  return result;
}

} // namespace Algorithms
} // namespace Mantid
