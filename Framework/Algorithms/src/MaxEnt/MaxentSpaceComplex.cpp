// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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
std::vector<double> MaxentSpaceComplex::toComplex(const std::vector<double> &values) { return values; }

/**
 * Converts a vector of complex numbers to a vector of complex numbers (i.e.
 * does nothing)
 * @param values : [input] The complex values as a vector
 * @return : The input as a vector of real numbers
 */
std::vector<double> MaxentSpaceComplex::fromComplex(const std::vector<double> &values) { return values; }

} // namespace Algorithms
} // namespace Mantid
