#include "MantidAlgorithms/MaxEnt/MaxentSpaceComplex.h"
#include <cmath>

namespace Mantid {
namespace Algorithms {

/**
 * Converts a vector of complex values to a vector of complex numbers (i.e. does
 * nothing)
 * @param values : [input] The complex values as a vector
 * @return : The input as a vector of complex numbers
 */
std::vector<double>
MaxentSpaceComplex::toComplex(const std::vector<double> &values) {

  return values;
}

/**
 * Converts a vector of complex numbers to a vector of complex numbers (i.e.
 * does nothing)
 * @param values : [input] The complex values as a vector
 * @return : The input as a vector of real numbers
 */
std::vector<double>
MaxentSpaceComplex::fromComplex(const std::vector<double> &values) {

  return values;
}

} // namespace Algorithms
} // namespace Mantid
