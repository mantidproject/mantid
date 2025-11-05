// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include <cstddef>

namespace Mantid {
namespace API {

/**
 * @brief The Correction struct to be applied as factor * TOF + offset
 * multiplicativeFactor:  TOF correction factor to multiply TOF by
 * additiveOffset:: TOF additive offset in unit of TOF
 */
struct Correction {
  Correction(const double &multiplicativeFactor, const double &additiveOffset)
      : factor(multiplicativeFactor), offset(additiveOffset) {}
  double factor;
  double offset;
};

/**
 * TimeAtSampleStrategy : Strategy (technique dependent) for determining Time At Sample
 *
 * SampleT = PulseT + [TOF to sample]
 */
class MANTID_API_DLL TimeAtSampleStrategy {
public:
  virtual Correction calculate(const size_t &workspace_index) const = 0;
  virtual ~TimeAtSampleStrategy() = default;
};

} // namespace API
} // namespace Mantid
