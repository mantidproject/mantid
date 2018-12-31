// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGY_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGY_H_

#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/**
 * @brief The Correction struct to be applied as factor * TOF + offset
 * offset:: TOF offset in unit of TOF
 * factor:  TOF correction factor to multiply with
 */
struct Correction {
  Correction(double tOffset, double tFactor)
      : offset(tOffset), factor(tFactor) {}
  double offset;
  double factor;
};

/** TimeAtSampleStrategy : Strategy (technique dependent) for determining Time
  At Sample

  SampleT = PulseT + [TOF to sample]
*/
class DLLExport TimeAtSampleStrategy {
public:
  virtual Correction calculate(const size_t &workspace_index) const = 0;
  virtual ~TimeAtSampleStrategy() = default;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGY_H_ */
