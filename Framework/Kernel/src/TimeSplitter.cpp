// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/TimeSplitter.h"

namespace Mantid {

namespace {
constexpr int IGNORE{-1};

void assertIncreasing(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop) {
  if (start > stop)
    throw std::runtime_error("TODO message");
}
} // namespace

namespace Kernel {

TimeSplitter::TimeSplitter(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop) {
  assertIncreasing(start, stop);
  m_roi_map.insert({start, 0});
  m_roi_map.insert({stop, IGNORE});
}

void TimeSplitter::addROI(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop,
                          const int value) {
  // cache what the final value will be
  const int endValue = this->valueAtTime(stop);

  // find if there are values to erase

  // the starting point is greater than or equal to the "start" supplied
  auto startIterator = m_roi_map.lower_bound(start);
  if ((startIterator->first != start) && (startIterator != m_roi_map.begin()))
    startIterator--; // move to the one before
  // the end is one past the "stop"
  auto stopIterator = m_roi_map.upper_bound(stop);

  // remove the elements that are being replaced [inclusive, exclusive)
  m_roi_map.erase(startIterator, stopIterator);

  // put in the new elements
  m_roi_map.insert({start, value});
  m_roi_map.insert({stop, endValue});
}

int TimeSplitter::valueAtTime(const Types::Core::DateAndTime &time) const {
  if (m_roi_map.empty())
    return IGNORE;
  if ((time < m_roi_map.begin()->first) || (time >= m_roi_map.rbegin()->first))
    return IGNORE;

  // find location that is greater than or equal to the requested time
  auto location = m_roi_map.lower_bound(time);
  if (location->first == time) {
    // found the time in the map
    return location->second;
  } else if (location == m_roi_map.begin()) {
    // iterator is greater than the first value in the map b/c equal is already handled
    // asked for a time outside of the map
    return IGNORE;
  } else {
    // go to the value before
    location--;
    return location->second;
  }
}

} // namespace Kernel
} // namespace Mantid
