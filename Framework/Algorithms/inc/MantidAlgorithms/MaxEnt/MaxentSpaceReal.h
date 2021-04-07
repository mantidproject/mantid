// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"

namespace Mantid {
namespace Algorithms {

/** MaxentSpaceReal : Defines the space of real numbers.
 */
class MANTID_ALGORITHMS_DLL MaxentSpaceReal : public MaxentSpace {
public:
  // Converts a real vector to a complex vector
  std::vector<double> toComplex(const std::vector<double> &values) override;
  // Converts a complex vector to a real vector
  std::vector<double> fromComplex(const std::vector<double> &values) override;
};

using MaxentSpaceReal_sptr = std::shared_ptr<Mantid::Algorithms::MaxentSpaceReal>;

} // namespace Algorithms
} // namespace Mantid
