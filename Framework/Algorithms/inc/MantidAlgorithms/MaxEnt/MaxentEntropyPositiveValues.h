// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/MaxEnt/MaxentEntropy.h"

namespace Mantid {
namespace Algorithms {

/** MaxentEntropyPositiveValues : Class defining the entropy of a positive image
  (i.e. a set of positive numbers). See J. Skilling and R. K Bryan, "Maximum
  entropy image reconstruction: general algorithm".
*/
class MANTID_ALGORITHMS_DLL MaxentEntropyPositiveValues : public MaxentEntropy {
public:
  // First derivative
  std::vector<double> derivative(const std::vector<double> &values, double background) override;
  // Second derivative
  std::vector<double> secondDerivative(const std::vector<double> &values, double background) override;
  // Correct negative values
  std::vector<double> correctValues(const std::vector<double> &values, double newValue) override;
};

} // namespace Algorithms
} // namespace Mantid
