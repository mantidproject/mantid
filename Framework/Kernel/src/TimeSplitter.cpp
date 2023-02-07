// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/TimeSplitter.h"
#include <set>

namespace Mantid {

using Types::Core::DateAndTime;

namespace {
// every value less than zero is ignore
constexpr int IGNORE{-1};
// default value for the output is zero
constexpr int DEFAULT_VALUE{0};

void assertIncreasing(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop) {
  if (start > stop)
    throw std::runtime_error("TODO message");
}
} // namespace

namespace Kernel {

TimeSplitter::TimeSplitter(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop) {
  clearAndReplace(start, stop, DEFAULT_VALUE);
}

void TimeSplitter::addROI(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop,
                          const int value) {
  assertIncreasing(start, stop);
  if (m_roi_map.empty()) {
    // set the values without checks
    clearAndReplace(start, stop, value);
  } else if ((start <= m_roi_map.begin()->first) && (stop >= m_roi_map.rbegin()->first)) {
    // overwrite existing map
    clearAndReplace(start, stop, value);
  } else {
    // do the interesting version

    // cache what the final value will be
    const int stopValue = this->valueAtTime(stop);

    // find if there are values to erase

    // the starting point is greater than or equal to the "start" supplied
    auto startIterator = m_roi_map.lower_bound(start);
    if ((startIterator->first != start) && (startIterator != m_roi_map.begin()))
      startIterator--; // move to the one before
    // the end is one past the "stop"
    auto stopIterator = m_roi_map.upper_bound(stop);
    if ((stopIterator != m_roi_map.end()) && (stopValue == IGNORE))
      stopIterator++; // move to the one after

    const bool atStart = (startIterator == m_roi_map.begin());

    // remove the elements that are being replaced [inclusive, exclusive)
    m_roi_map.erase(startIterator, stopIterator);

    // put in the new elements
    if ((value > IGNORE) || (!atStart))
      m_roi_map.insert({start, value});

    if (value != stopValue)
      m_roi_map.insert({stop, stopValue});

    // verify this ends with IGNORE
    if (m_roi_map.rbegin()->second != IGNORE)
      throw std::runtime_error("Something went wrong in TimeSplitter::addROI");
  }
}

void TimeSplitter::clearAndReplace(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop,
                                   const int value) {
  m_roi_map.clear();
  if (value >= 0) {
    m_roi_map.insert({start, value});
    m_roi_map.insert({stop, IGNORE});
  }
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

/**
 * Return a sorted vector of the output workspace indices
 */
std::vector<int> TimeSplitter::outputWorkspaceIndices() const {
  // sets have unique values and are sorted
  std::set<int> outputSet;

  // copy all of the (not ignore) output workspace indices
  for (const auto iter : m_roi_map) {
    if (iter.second > IGNORE)
      outputSet.insert(iter.second);
  }

  // return a vector
  return std::vector<int>(outputSet.begin(), outputSet.end());
}

std::size_t TimeSplitter::numRawValues() const { return m_roi_map.size(); }

} // namespace Kernel
} // namespace Mantid
