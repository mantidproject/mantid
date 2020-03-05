// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_EXCLUDERANGEFINDER_H_
#define MANTID_CURVEFITTING_EXCLUDERANGEFINDER_H_

#include "MantidCurveFitting/DllConfig.h"

#include <vector>

namespace Mantid {
namespace CurveFitting {

/** ExcludeRangeFinder : Helper clss that finds if a point should be excluded
from fit. It keeps the boundaries of the relevant exclusion region for the last
checked value. A relevant region is the one which either includes the value or
the nearest one with the left boundary greater than the value. The class also
keeps the index of the region (its left boundary) for efficient search.
*/
class DLLExport ExcludeRangeFinder {
public:
  /// Constructor
  ExcludeRangeFinder(const std::vector<double> &exclude, double startX,
                     double endX);

  /// Check if an x-value lies in an exclusion range
  bool isExcluded(double value);

private:
  /// Find the range from m_exclude that may contain points x >= p
  void findNextExcludedRange(double p);
  /// Index of current excluded range
  size_t m_exclIndex;
  /// Start of current excluded range
  double m_startExcludedRange;
  /// End of current excluded range
  double m_endExcludeRange;
  /// Reference to a list of exclusion ranges.
  const std::vector<double> m_exclude;
  /// Size of m_exclude.
  const size_t m_size;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_EXCLUDERANGEFINDER_H_ */
