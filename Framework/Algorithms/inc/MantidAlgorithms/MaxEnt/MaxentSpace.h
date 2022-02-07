// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include <memory>
#include <vector>

namespace Mantid {
namespace Algorithms {

/** MaxentSpace : Abstract base class defining a MaxentSpace. A Maxent space can
  be the space of real numbers or the space of complex numbers at the moment.
  This class is intended to be used in conjunction with MaxentTransformFourier,
  to define the input and output to/from the Fourier transform.
*/
class MANTID_ALGORITHMS_DLL MaxentSpace {
public:
  // Constructor
  MaxentSpace() = default;
  // Destructor
  virtual ~MaxentSpace() = default;
  // Converts a given vector to a complex vector
  virtual std::vector<double> toComplex(const std::vector<double> &values) = 0;
  // Converts to a complex vector
  virtual std::vector<double> fromComplex(const std::vector<double> &values) = 0;
};

using MaxentSpace_sptr = std::shared_ptr<MaxentSpace>;

} // namespace Algorithms
} // namespace Mantid
