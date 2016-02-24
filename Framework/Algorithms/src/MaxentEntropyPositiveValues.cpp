#include "MantidAlgorithms/MaxentEntropyPositiveValues.h"

namespace Mantid {
namespace Algorithms {

double MaxentEntropyPositiveValues::getDerivative(double value) {

  return (-log(value));
}

double MaxentEntropyPositiveValues::getSecondDerivative(double value) {

  return value;
}

double MaxentEntropyPositiveValues::correctValue(double value,
                                                 double newValue) {

  if (value < 0)
    value = newValue;

  return value;
}

} // namespace Algorithms
} // namespace Mantid
