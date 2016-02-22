#include "MantidAlgorithms/MaxentEntropyNegativeValues.h"

namespace Mantid {
namespace Algorithms {

double MaxentEntropyNegativeValues::getDerivative(double value) {

  return (-log(value + std::sqrt(value * value + 1)));
}

double MaxentEntropyNegativeValues::getSecondDerivative(double value) {

  // TODO: modify this and use the correct expression, i.e. second derivative
  return fabs(value);
}

double MaxentEntropyNegativeValues::correctValue(double value) {

  // Nothing to correct
  return value;
}

} // namespace Algorithms
} // namespace Mantid
