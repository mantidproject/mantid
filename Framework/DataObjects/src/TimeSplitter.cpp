// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataObjects/TimeSplitter.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeROI.h"

#include <set>

namespace Mantid {
using Kernel::TimeROI;
using Types::Core::DateAndTime;

namespace DataObjects {

namespace {
// every value less than zero is ignore
constexpr int IGNORE_VALUE{-1};
// default value for the output is zero
constexpr int DEFAULT_VALUE{0};

void assertIncreasing(const DateAndTime &start, const DateAndTime &stop) {
  if (start > stop)
    throw std::runtime_error("TODO message");
}

/// static Logger definition
Kernel::Logger g_log("TimeSplitter");

} // namespace

TimeSplitter::TimeSplitter(const DateAndTime &start, const DateAndTime &stop) {
  clearAndReplace(start, stop, DEFAULT_VALUE);
}

std::string TimeSplitter::debugPrint() const {
  std::stringstream msg;
  for (const auto iter : m_roi_map)
    msg << iter.second << "|" << iter.first << "\n";
  return msg.str();
}

void TimeSplitter::addROI(const DateAndTime &start, const DateAndTime &stop, const int value) {
  assertIncreasing(start, stop);
  if (m_roi_map.empty()) {
    // set the values without checks
    clearAndReplace(start, stop, value);
  } else if ((start <= m_roi_map.begin()->first) && (stop >= m_roi_map.rbegin()->first)) {
    // overwrite existing map
    clearAndReplace(start, stop, value);
  } else if ((stop < m_roi_map.begin()->first) || (start > m_roi_map.rbegin()->first)) {
    // adding to one end or the other
    if (value > IGNORE_VALUE) { // only add non-ignore values
      m_roi_map.insert({start, value});
      m_roi_map.insert({stop, IGNORE_VALUE});
    }
  } else {
    // do the interesting version
    g_log.debug() << "addROI(" << start << ", " << stop << ", " << value << ")\n";

    // cache what the final value will be
    const int stopValue = this->valueAtTime(stop);

    // find if there are values to erase

    // the starting point is greater than or equal to the "start" supplied
    auto startIterator = m_roi_map.lower_bound(start);
    if ((startIterator->first != start) && (startIterator != m_roi_map.begin()))
      startIterator--; // move to the one before

    // the end is one past the "stop"
    auto stopIterator = m_roi_map.upper_bound(stop);
    if ((stopIterator != m_roi_map.end()) && (stopValue == IGNORE_VALUE))
      stopIterator++; // move to the one after

    const bool atStart = (startIterator == m_roi_map.begin());

    // remove the elements that are being replaced [inclusive, exclusive)
    m_roi_map.erase(startIterator, stopIterator);

    // put in the new elements
    if ((value > IGNORE_VALUE) || (!atStart)) {
      if (value != this->valueAtTime(start)) {
        m_roi_map.insert({start, value});
      }
    }

    // find the new iterator for where this goes to see if it the same value
    stopIterator = m_roi_map.lower_bound(stop);
    if ((stopIterator != m_roi_map.end()) && (value == stopIterator->second)) {
      m_roi_map.erase(stopIterator);
    }
    if (value != stopValue) {
      m_roi_map.insert({stop, stopValue});
    }

    // verify this ends with IGNORE_VALUE
    if (m_roi_map.rbegin()->second != IGNORE_VALUE) {
      throw std::runtime_error("Something went wrong in TimeSplitter::addROI");
    }
  }
}

void TimeSplitter::clearAndReplace(const DateAndTime &start, const DateAndTime &stop, const int value) {
  m_roi_map.clear();
  if (value >= 0) {
    m_roi_map.insert({start, value});
    m_roi_map.insert({stop, IGNORE_VALUE});
  }
}

int TimeSplitter::valueAtTime(const DateAndTime &time) const {
  if (m_roi_map.empty())
    return IGNORE_VALUE;
  if (time < m_roi_map.begin()->first)
    return IGNORE_VALUE;

  // this method can be used when the object is in an unusual state and doesn't
  // end with IGNORE_VALUE

  // find location that is greater than or equal to the requested time and give
  // back previous value
  auto location = m_roi_map.lower_bound(time);
  if (location->first == time) {
    // found the time in the map
    return location->second;
  } else if (location == m_roi_map.begin()) {
    // iterator is greater than the first value in the map b/c equal is already
    // handled asked for a time outside of the map
    return IGNORE_VALUE;
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
    if (iter.second > IGNORE_VALUE)
      outputSet.insert(iter.second);
  }

  // return a vector
  return std::vector<int>(outputSet.begin(), outputSet.end());
}

/**
 * Returns a Mantid::Kernel::TimeROI for the requested workspace index.
 * This will raise an exception if the workspace index does not exist in the TimeSplitter.
 */
TimeROI TimeSplitter::getTimeROI(const int workspaceIndex) {
  // convert things less than -1 to -1
  const int effectiveIndex = std::max<int>(workspaceIndex, -1);

  TimeROI output;
  using map_value_type = std::map<DateAndTime, int>::value_type;
  auto indexFinder = [effectiveIndex](const map_value_type &value) { return value.second == effectiveIndex; };
  // find the first place this workspace index exists
  auto iter = std::find_if(m_roi_map.begin(), m_roi_map.end(), indexFinder);
  // add the ROI found then loop until we reach the end
  while (iter != m_roi_map.end()) {
    // add the ROI
    const auto startTime = iter->first;
    iter++;
    // if the next iterator is the end there is nothing to add
    if (iter != m_roi_map.end()) {
      const auto stopTime = iter->first;
      output.addROI(startTime, stopTime);
    }

    // look for the next place the workspace index occurs
    iter = std::find_if(iter, m_roi_map.end(), indexFinder);
  }

  // error check that something is there
  // ignore index being empty is ok
  if ((workspaceIndex >= 0) && (output.empty())) {
    std::stringstream msg;
    msg << "No regions exist for workspace index " << workspaceIndex;
  }

  return output;
}

std::size_t TimeSplitter::numRawValues() const { return m_roi_map.size(); }

} // namespace DataObjects
} // namespace Mantid