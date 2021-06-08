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

/** MaxentEntropyNegativeValues : Class defining the entropy of a 'PosNeg' image
  (i.e. a set of real numbers). References:
        1. A. J. Markvardsen, "Polarised neutron diffraction measurements of
  PrBa2Cu3O6+x and the Bayesian statistical analysis of such data".
        2. P. F. Smith and M. A. Player, "Deconvolution of bipolar ultrasonic
  signals using a modified maximum entropy method"
*/
class MANTID_ALGORITHMS_DLL MaxentEntropyNegativeValues : public MaxentEntropy {
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
