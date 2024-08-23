// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/ExcludeRangeFinder.h"

#include <algorithm>

namespace Mantid::CurveFitting {

/// Constructor.
/// @param exclude :: The value of the "Exclude" property.
/// @param startX :: The start of the overall fit interval.
/// @param endX :: The end of the overall fit interval.
ExcludeRangeFinder::ExcludeRangeFinder(const std::vector<double> &exclude, double startX, double endX)
    : m_exclIndex(exclude.size()), m_startExcludedRange(), m_endExcludeRange(), m_exclude(exclude),
      m_size(exclude.size()) {
  // m_exclIndex is initialised with exclude.size() to be the default when
  // there are no exclusion ranges defined.
  if (!m_exclude.empty()) {
    if (startX < m_exclude.back() && endX > m_exclude.front()) {
      // In this case there are some ranges, the index starts with 0
      // and first range is found.
      m_exclIndex = 0;
      findNextExcludedRange(startX);
    }
  }
}

/// Check if an x-value lies in an exclusion range.
/// @param value :: A value to check.
/// @returns true if the value lies in an exclusion range and should be
/// excluded from fit.
bool ExcludeRangeFinder::isExcluded(double value) {
  if (m_exclIndex < m_size) {
    if (value < m_startExcludedRange) {
      // If a value is below the start of the current interval
      // it is not in any other interval by the workings of
      // findNextExcludedRange
      return false;
    } else if (value <= m_endExcludeRange) {
      // value is inside
      return true;
    } else {
      // Value is past the current range. Find the next one or set the index
      // to m_exclude.size() to stop further searches.
      findNextExcludedRange(value);
      // The value can find itself inside another range.
      return isExcluded(value);
    }
  }
  return false;
}

/// Find the range from m_exclude that may contain points x >= p .
/// @param p :: An x value to use in the seach.
void ExcludeRangeFinder::findNextExcludedRange(double p) {
  if (p > m_exclude.back()) {
    // If the value is past the last point stop any searches or checks.
    m_exclIndex = m_size;
    return;
  }
  // Starting with the current index m_exclIndex find the first value in
  // m_exclude that is greater than p. If this point is a start than the
  // end will be the following point. If it's an end then the start is
  // the previous point. Keep index m_exclIndex pointing to the start.
  const auto it = std::find_if(m_exclude.cbegin(), m_exclude.cend(), [&p](const auto &ex) { return ex >= p; });
  if (it == m_exclude.cend())
    return;
  m_exclIndex = static_cast<std::size_t>(std::distance(m_exclude.begin(), it));
  if (m_exclIndex % 2 == 0) {
    // A number at an even position in m_exclude starts an exclude
    // range
    m_startExcludedRange = *it;
    m_endExcludeRange = *(it + 1);
  } else {
    // A number at an odd position in m_exclude ends an exclude range
    m_startExcludedRange = *(it - 1);
    m_endExcludeRange = *it;
    --m_exclIndex;
  }
  // No need for additional checks as p < m_exclude.back()
  // and m_exclude[m_exclIndex] < p due to conditions at the calls
  // so the break statement will always be reached.
}
} // namespace Mantid::CurveFitting
