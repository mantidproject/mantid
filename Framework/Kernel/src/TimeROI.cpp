// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include <iostream>
#include <limits>

#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeROI.h"

namespace Mantid {
namespace Kernel {

using Mantid::Types::Core::DateAndTime;
// alias for iterators
using DateAndTimeIter = std::vector<DateAndTime>::iterator;

namespace {
/// static Logger definition
Logger g_log("TimeROI");

const bool ROI_USE{true};
const bool ROI_IGNORE{false};

/// @throws std::runtime_error if not in increasing order
void assert_increasing(const DateAndTime &startTime, const DateAndTime &stopTime) {
  if (!bool(startTime < stopTime)) {
    std::stringstream msg;
    msg << startTime << " and  " << stopTime << " are not in increasing order";
    throw std::runtime_error(msg.str());
  }
}
} // namespace

const std::string TimeROI::NAME = "Kernel_TimeROI";

TimeROI::TimeROI() {}

TimeROI::TimeROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  this->addROI(startTime, stopTime);
}

TimeROI::TimeROI(const Kernel::TimeSeriesProperty<bool> *filter) { this->replaceROI(filter); }

void TimeROI::addROI(const std::string &startTime, const std::string &stopTime) {
  this->addROI(DateAndTime(startTime), DateAndTime(stopTime));
}

/**
 * Add new region as a union.
 */
void TimeROI::addROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  assert_increasing(startTime, stopTime);

  if ((this->empty()) || (startTime > m_roi.back())) {
    // add in the new region
    m_roi.push_back(startTime);
    m_roi.push_back(stopTime);
  } else if (stopTime < m_roi.front()) {
    m_roi.insert(m_roi.begin(), stopTime);
    m_roi.insert(m_roi.begin(), startTime);
  } else {
    const bool startValueOld = valueAtTime(startTime);
    const bool stopValueOld = valueAtTime(stopTime);

    DateAndTimeIter startIter;
    if (startValueOld == ROI_IGNORE) {
      // expanding into unused region
      startIter = std::upper_bound(m_roi.begin(), m_roi.end(), startTime);
    } else {
      startIter = std::lower_bound(m_roi.begin(), m_roi.end(), startTime);
    }

    DateAndTimeIter stopIter;
    if (stopValueOld == ROI_USE) {
      stopIter = std::lower_bound(startIter, m_roi.end(), stopTime);
    } else {
      stopIter = std::upper_bound(startIter, m_roi.end(), stopTime);
    }

    if (startIter == stopIter) {
      if (startValueOld == ROI_USE) {
        g_log.debug("TimeROI::addROI is already accounted for. Addition is being ignored");
      } else {
        // move the start time
        *startIter = startTime;
      }
    } else {
      const bool addTwo = bool(std::distance(startIter, stopIter) % 2 == 0);
      auto insertPos = m_roi.erase(startIter, stopIter);
      if (addTwo) {
        m_roi.insert(insertPos, stopTime);
        m_roi.insert(insertPos, startTime);
      } else {
        if (startValueOld == ROI_IGNORE)
          m_roi.insert(insertPos, startTime);
        else
          m_roi.insert(insertPos, stopTime);
      }
    }
  }

  // verify "this" is in a good state
  this->validateValues("TimeROI::addROI");
}

void TimeROI::addROI(const std::time_t &startTime, const std::time_t &stopTime) {
  this->addROI(DateAndTime(startTime), DateAndTime(stopTime));
}

void TimeROI::addMask(const std::string &startTime, const std::string &stopTime) {
  this->addMask(DateAndTime(startTime), DateAndTime(stopTime));
}

/**
 * Remove a region that is already in use.
 *
 * This subtracts the intersection which means adding a mask to an empty area does nothing. This may leave redundant
 * values in the ROI
 */
void TimeROI::addMask(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  assert_increasing(startTime, stopTime);

  if (this->empty()) {
    g_log.debug("TimeROI::addMask to an empty object is ignored");
  } else if ((startTime > m_roi.back()) || (stopTime < m_roi.front())) {
    g_log.debug("TimeROI::addMask to ignored region");
  } else if ((startTime <= m_roi.front()) && (stopTime >= m_roi.back())) {
    // the mask includes everything so remove all current values
    this->m_roi.clear();
  } else if ((startTime <= m_roi.front()) && (stopTime < m_roi.back())) {
    // trimming the front off
    auto iter = std::upper_bound(m_roi.begin(), m_roi.end(), stopTime);
    const auto newValue = std::max(m_roi.front(), stopTime);
    m_roi.erase(m_roi.begin(), iter);
    // decide how to put in the new start time based on whether this has an even number of values
    if (m_roi.size() % 2 == 0)
      *(m_roi.begin()) = newValue;
    else
      m_roi.insert(m_roi.begin(), newValue);
  } else if ((startTime > m_roi.front()) && (stopTime >= m_roi.back())) {
    // trim the end off
    auto iter = std::upper_bound(m_roi.begin(), m_roi.end(), startTime);
    const auto newValue = std::min(*iter, startTime);
    iter = m_roi.erase(iter, m_roi.end());
    // decide how to put in the new start time based on whether this has an even number of values
    if (m_roi.size() % 2 == 0)
      *iter = newValue;
    else
      m_roi.push_back(newValue);
  } else {
    g_log.debug("TimeROI::addMask cutting notch in existing ROI");
    // cutting a notch in an existing ROI
    auto firstIter = std::lower_bound(m_roi.begin(), m_roi.end(), startTime);
    auto lastIter = std::lower_bound(m_roi.begin(), m_roi.end(), stopTime);
    if (firstIter == lastIter) {
      if (std::distance(m_roi.begin(), firstIter) % 2 == 1) {
        // completely in a USE region add the stop time first
        auto newPos = m_roi.insert(firstIter, stopTime);
        m_roi.insert(newPos, startTime);
      } else {
        g_log.debug("TimeROI::addMask cutting notch in existing ignore region doing nothing");
      }
    } else {
      // all other cases are a bit more involved
      if (std::distance(m_roi.begin(), firstIter) % 2 == 0) {
        // moving the starting point because it is in an ignore region
        firstIter = std::upper_bound(m_roi.begin(), lastIter, startTime);
      }
      if (std::distance(m_roi.begin(), lastIter) % 2 == 0) {
        // moving the ending point because it is in a use region
        firstIter = std::upper_bound(firstIter, m_roi.end(), stopTime);
      }
      // remove values
      firstIter = m_roi.erase(firstIter, lastIter);
      // add in new part - stop first so the insertion iterator can be reused
      m_roi.insert(firstIter, stopTime);
      m_roi.insert(firstIter, startTime);
    }
  }
  // verify "this" is in a good state
  this->validateValues("TimeROI::addMask");
}

void TimeROI::addMask(const std::time_t &startTime, const std::time_t &stopTime) {
  this->addMask(DateAndTime(startTime), DateAndTime(stopTime));
}

/**
 * This method returns true if the entire region between startTime and stopTime is inside an existing interval.
 * If part of the supplied region is not covered this returns false.
 */
bool TimeROI::isCompletelyInROI(const Types::Core::DateAndTime &startTime,
                                const Types::Core::DateAndTime &stopTime) const {
  // check if the region is in the overall window at all
  if ((startTime > m_roi.back()) || (stopTime < m_roi.front()))
    return false;

  // since the ROI should be alternating "use" and "ignore", see if the start and stop are within a single region
  const auto iterStart = std::lower_bound(m_roi.cbegin(), m_roi.cend(), startTime);
  const auto iterStop = std::lower_bound(iterStart, m_roi.cend(), stopTime);
  // too far apart
  if (std::distance(iterStart, iterStop) > 0)
    return false;

  // the value at the start time should be "use"
  return this->valueAtTime(startTime);
}

/**
 * This returns whether the time should be "used" (rather than ignored).
 * Anything outside of the region of interest is ignored.
 *
 * The value is, essentially, whatever it was at the last recorded time before or equal to the one requested.
 */
bool TimeROI::valueAtTime(const DateAndTime &time) const {
  if (this->empty() || time < m_roi.front() || time >= m_roi.back()) {
    // ignore everything outside of range
    return ROI_IGNORE;
  } else {
    // find first value greater than this one
    const auto iterUpper = std::upper_bound(m_roi.cbegin(), m_roi.cend(), time);

    // check if it is on a boundary
    if (std::find(iterUpper, m_roi.cend(), time) != m_roi.cend()) {
      if (std::distance(m_roi.cbegin(), iterUpper) % 2 == 0)
        return ROI_USE;
      else
        return ROI_IGNORE;
    }

    // find the first value lower than this
    const auto iterLower = std::lower_bound(iterUpper, m_roi.cend(), time);

    // use the value of that iterator
    if (std::distance(m_roi.cbegin(), iterLower) % 2 == 0)
      return ROI_IGNORE;
    else
      return ROI_USE;
  }
}

/// get a list of all unique times. order is not guaranteed
std::vector<DateAndTime> TimeROI::getAllTimes(const TimeROI &other) {

  std::set<DateAndTime> times_set;
  times_set.insert(this->m_roi.cbegin(), this->m_roi.cend());
  times_set.insert(other.m_roi.cbegin(), other.m_roi.cend());

  // copy into the vector
  std::vector<DateAndTime> times_all;
  times_all.assign(times_set.begin(), times_set.end());

  return times_all;
}

void TimeROI::replaceROI(const TimeSeriesProperty<bool> *roi) {
  // this is used by LogManager::loadNexus
  m_roi.clear();

  if (roi->size() > 0) {
    // make a copy with unique values
    TimeSeriesProperty<bool> roi_copy(*roi);
    roi_copy.eliminateDuplicates(); // takes last value

    // TSP will always give the times sorted in increasing order and the values parallel to that
    const auto values = roi_copy.valuesAsVector();

    // find the first USE value and start there
    const auto iter = std::find(values.cbegin(), values.cend(), ROI_USE);
    std::size_t start = std::size_t(std::distance(values.cbegin(), iter));

    const auto times = roi_copy.timesAsVector();

    // add the first start time
    m_roi.push_back(times[start]);
    start++; // advance past that

    // add values
    const auto NUM_VALUES = values.size();
    for (std::size_t i = start; i < NUM_VALUES; ++i) {
      if (values[i - 1] == values[i])
        continue; // skip if the value is the same as the predicessor
      // add the value to the end
      m_roi.push_back(times[i]);
    }
  }

  this->validateValues("TimeROI::replaceROI");
}

void TimeROI::replaceROI(const TimeROI &other) {
  m_roi.clear();
  m_roi.assign(other.m_roi.cbegin(), other.m_roi.cend());
}

/**
 * Updates the TimeROI values with the union with another TimeROI.
 * See https://en.wikipedia.org/wiki/Union_(set_theory) for more details
 *
 * This will remove redundant entries as a side-effect.
 */
void TimeROI::update_union(const TimeROI &other) {
  // exit early if the two TimeROI are identical
  if (*this == other)
    return;

  // add all the intervals from the other
  for (const auto interval : other.toSplitters()) {
    this->addROI(interval.start(), interval.stop());
  }
}

/**
 * Updates the TimeROI values with the intersection with another TimeROI.
 * See https://en.wikipedia.org/wiki/Intersection for more details
 *
 * This will remove redundant entries as a side-effect.
 */
void TimeROI::update_intersection(const TimeROI &other) {
  // exit early if the two TimeROI are identical
  if (*this == other)
    return;

  // remove everything before the other starts
  if (m_roi.front() < other.m_roi.front()) {
    this->addMask(m_roi.front(), other.m_roi.front());
  }

  // add the spaces between the other's splitting intervals
  const std::size_t LOOP_MAX = std::size_t(other.m_roi.size() - 1);
  for (std::size_t i = 1; i < LOOP_MAX; i += 2) {
    this->addMask(other.m_roi[i], other.m_roi[i + 1]);
  }

  // remove everything after other finishes
  if (m_roi.back() > other.m_roi.back()) {
    this->addMask(other.m_roi.back(), m_roi.back());
  }
}

/**
 * This method is to lend itself to be compatible with existing implementation
 */
const std::vector<SplittingInterval> TimeROI::toSplitters() const {
  const auto NUM_VAL = m_roi.size();
  std::vector<SplittingInterval> output;
  // every other value is a start/stop
  for (std::size_t i = 0; i < NUM_VAL; i += 2) {
    output.push_back({m_roi[i], m_roi[i + 1]});
  }

  return output;
}

bool TimeROI::operator==(const TimeROI &other) const { return this->m_roi == other.m_roi; }

void TimeROI::debugPrint(const std::size_t type) const {
  if (type == 0) {
    const auto NUM_VALUES{m_roi.size()};
    for (std::size_t i = 0; i < NUM_VALUES; i += 2) {
      std::cout << (i / 2) << ": " << m_roi[i] << " to " << m_roi[i + 1] << std::endl;
    }
  } else if (type == 1) {
    for (const auto val : m_roi)
      std::cout << val << " ";
    std::cout << std::endl;
  } else {
    throw std::runtime_error("Invalid type parameter");
  }
}

size_t TimeROI::getMemorySize() const { return this->numBoundaries() * sizeof(DateAndTime); }

/**
 * Duration of the whole TimeROI
 */
double TimeROI::durationInSeconds() const {
  const auto ROI_SIZE = this->numBoundaries();
  if (ROI_SIZE == 0) {
    return 0.;
  } else {
    double total{0.};
    for (std::size_t i = 0; i < ROI_SIZE - 1; i += 2) {
      total += DateAndTime::secondsFromDuration(m_roi[i + 1] - m_roi[i]);
    }

    return total;
  }
}

/**
 * Duration of the TimeROI between startTime and stopTime
 */
double TimeROI::durationInSeconds(const Types::Core::DateAndTime &startTime,
                                  const Types::Core::DateAndTime &stopTime) const {
  assert_increasing(startTime, stopTime);

  // just return the difference between the supplied other times if this has no regions
  if (this->empty()) {
    return DateAndTime::secondsFromDuration(stopTime - startTime);
  }

  if (stopTime <= m_roi.front()) { // asking before ROI
    return 0.;
  } else if (startTime >= m_roi.back()) { // asking after ROI
    return 0.;
  } else if ((startTime <= m_roi.front()) && (stopTime >= m_roi.back())) { // full range of ROI
    return this->durationInSeconds();
  } else { // do the calculation
    // the time requested is an intersection of start/stop time and this object
    TimeROI temp{startTime, stopTime};
    temp.update_intersection(*this);
    return temp.durationInSeconds();
  }
}

void TimeROI::validateValues(const std::string &label) {
  // verify there is an even number of values
  if (m_roi.size() % 2 != 0) {
    std::stringstream msg;
    msg << "Something went wrong in " << label << " and the object is now invalid. "
        << "There are " << m_roi.size() << " regions and it must be an even number";
    throw std::runtime_error(msg.str());
  }
  // verify the values are in increasing order
  if (!std::is_sorted(m_roi.cbegin(), m_roi.cend())) {
    throw std::runtime_error("Values are not in increasing order");
  }
  // verify the values are unique
  const std::size_t NUM_UNIQUE = std::size_t(std::distance(m_roi.begin(), std::unique(m_roi.begin(), m_roi.end())));
  if (NUM_UNIQUE != m_roi.size()) {
    throw std::runtime_error("Values are not unique");
  }
}

std::size_t TimeROI::numBoundaries() const { return static_cast<std::size_t>(m_roi.size()); }

bool TimeROI::empty() const { return bool(this->numBoundaries() == 0); }

// serialization / deserialization items
void TimeROI::saveNexus(::NeXus::File *file) const {
  // create a local TimeSeriesProperty which will do the actual work
  TimeSeriesProperty<bool> tsp(NAME);
  for (const auto interval : this->toSplitters()) {
    tsp.addValue(interval.start(), ROI_USE);
    tsp.addValue(interval.stop(), ROI_IGNORE);
  }

  // save things to to disk
  tsp.saveProperty(file);
}

} // namespace Kernel
} // namespace Mantid
