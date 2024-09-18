// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include <iostream>
#include <limits>
#include <sstream>

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

/*
 * This method assumes that there is an overlap between the two intervals
 */
TimeInterval calculate_union(const TimeInterval &left, const TimeInterval &right) {
  return TimeInterval(std::min(left.start(), right.start()), std::max(left.stop(), right.stop()));
}

/*
 * This is slightly different than overlaps in TimeInterval because
 * two timeROI that touch on the boundary are considered overlapping
 */
bool overlaps(const TimeInterval &left, const TimeInterval &right) {
  if (left.overlaps(right))
    return true;
  else if (left.start() == right.stop())
    return true;
  else if (left.stop() == right.start())
    return true;
  else
    return false; // they don't overlap
}

} // namespace

const std::string TimeROI::NAME = "Kernel_TimeROI";
/// Constant for TimeROI where no time is used
const TimeROI TimeROI::USE_NONE{DateAndTime::GPS_EPOCH - DateAndTime::ONE_SECOND, DateAndTime::GPS_EPOCH};
/// Constant for TimeROI where any time is used
const TimeROI TimeROI::USE_ALL{};

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
  } else if (this->isCompletelyInROI(startTime, stopTime)) {
    g_log.debug("TimeROI::addROI to use region");
  } else if ((startTime <= m_roi.front()) && stopTime >= m_roi.back()) {
    // overwrite everything
    m_roi.clear();
    m_roi.push_back(startTime);
    m_roi.push_back(stopTime);
  } else if (stopTime < m_roi.front() || startTime > m_roi.back()) {
    m_roi.insert(m_roi.begin(), stopTime);
    m_roi.insert(m_roi.begin(), startTime);
  } else {
    TimeInterval roi_to_add(startTime, stopTime);
    std::vector<TimeInterval> output;
    bool union_added = false;
    for (const auto &interval : this->toTimeIntervals()) {
      if (overlaps(roi_to_add, interval)) {
        // the roi absorbs this interval
        // this check must be first
        roi_to_add = calculate_union(roi_to_add, interval);
      } else if (interval < roi_to_add) {
        output.push_back(interval);
      } else if (interval > roi_to_add) {
        if (!union_added) {
          output.push_back(roi_to_add);
          union_added = true;
        }
        output.push_back(interval);
      } else {
        throw std::runtime_error("encountered supposedly imposible place in TimeROI::addROI");
      }
    }
    if (!union_added)
      output.push_back(roi_to_add);
    this->clear();
    for (const auto interval : output) {
      m_roi.push_back(interval.start());
      m_roi.push_back(interval.stop());
    }
  }

  // verify "this" is in a good state
  this->validateValues("TimeROI::addROI");
}

void TimeROI::addROI(const std::time_t &startTime, const std::time_t &stopTime) {
  this->addROI(DateAndTime(startTime), DateAndTime(stopTime));
}

/**
 * Append a new region to TimeROI fast. Use this method only when there is no need for the comprehensive logic of
 * addROI.
 */
void TimeROI::appendROIFast(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  if (!m_roi.empty() && m_roi.back() == startTime)
    m_roi.back() = stopTime; // grow existent region
  else {                     // add new region
    m_roi.push_back(startTime);
    m_roi.push_back(stopTime);
  }
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

  if (this->useAll()) {
    g_log.debug("TimeROI::addMask to an empty object is ignored");
  } else if (this->isCompletelyInMask(startTime, stopTime)) {
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
    if (m_roi.size() % 2 == 0) {
      if (newValue > m_roi.front())
        *(m_roi.begin()) = newValue;
    } else {
      m_roi.insert(m_roi.begin(), newValue);
    }
  } else if ((startTime > m_roi.front()) && (stopTime >= m_roi.back())) {
    // trim the end off
    auto iter = std::upper_bound(m_roi.begin(), m_roi.end(), startTime);
    const auto newValue = std::min(*iter, startTime);
    iter = m_roi.erase(iter, m_roi.end());
    // decide how to put in the new start time based on whether this has an even number of values
    if (m_roi.size() % 2 == 0) {
      *iter = newValue;
    } else {
      if (newValue == m_roi.back()) {
        // currently ends in use and needs to be removed
        m_roi.pop_back();
      } else {
        // need to close the end
        m_roi.push_back(newValue);
      }
    }
  } else {
    g_log.debug("TimeROI::addMask cutting notch in existing ROI");
    // create rois for before and after the mask
    const TimeInterval use_before(m_roi.front(), startTime);
    const TimeInterval use_after(stopTime, m_roi.back());

    // loop through all current splitters and get new intersections
    std::vector<TimeInterval> output;
    TimeInterval intersection;
    const std::size_t roiSize = this->numBoundaries();
    for (std::size_t i = 0; i < roiSize; i += 2) {
      const TimeInterval interval(this->m_roi[i], this->m_roi[i + 1]);
      intersection = use_before.intersection(interval);
      if (intersection.isValid()) {
        output.push_back(intersection);
      }
      intersection = use_after.intersection(interval);
      if (intersection.isValid()) {
        output.push_back(intersection);
      }
    }
    this->clear();
    for (const auto &interval : output) {
      m_roi.push_back(interval.start());
      m_roi.push_back(interval.stop());
    }
  }

  // verify "this" is in a good state
  this->validateValues("TimeROI::addMask");
}

void TimeROI::addMask(const std::time_t &startTime, const std::time_t &stopTime) {
  this->addMask(DateAndTime(startTime), DateAndTime(stopTime));
}

/**
 * This method returns true if the entire region between startTime and stopTime is inside an existing use interval.
 * If part of the supplied region is not covered this returns false.
 */
bool TimeROI::isCompletelyInROI(const Types::Core::DateAndTime &startTime,
                                const Types::Core::DateAndTime &stopTime) const {
  // check if the region is in the overall window at all
  if ((startTime > m_roi.back()) || (stopTime <= m_roi.front()))
    return false;

  // since the ROI should be alternating "use" and "ignore", see if the start and stop are within a single region
  const auto iterStart = std::lower_bound(m_roi.cbegin(), m_roi.cend(), startTime);
  const auto iterStop = std::lower_bound(iterStart, m_roi.cend(), stopTime);
  // too far apart
  if (std::distance(iterStart, iterStop) > 0)
    return false;

  // the value at the start time should be "use"
  return this->valueAtTime(startTime) == ROI_USE;
}

/**
 * This method returns true if the entire region between startTime and stopTime is inside an existing ignore interval.
 * If part of the supplied region is not covered this returns false.
 */
bool TimeROI::isCompletelyInMask(const Types::Core::DateAndTime &startTime,
                                 const Types::Core::DateAndTime &stopTime) const {
  if (this->useAll())
    return true;
  if (startTime >= m_roi.back())
    return true;
  if (stopTime < m_roi.front())
    return true;

  const auto iterStart = std::lower_bound(m_roi.cbegin(), m_roi.cend(), startTime);
  const auto iterStop = std::lower_bound(iterStart, m_roi.cend(), stopTime);
  // too far apart
  if (std::distance(iterStart, iterStop) > 0)
    return false;

  // give the answer
  return this->valueAtTime(startTime) == ROI_IGNORE;
}

/**
 * This returns whether the time should be "used" (rather than ignored).
 * Anything outside of the region of interest is ignored.
 *
 * The value is, essentially, whatever it was at the last recorded time before or equal to the one requested.
 */
bool TimeROI::valueAtTime(const Types::Core::DateAndTime &time) const {
  if (useNone() || empty())
    return ROI_IGNORE;

  // ignore a time if it's outside ROI
  if (time < m_roi.front() || time >= m_roi.back())
    return ROI_IGNORE;

  // get the first ROI time boundary greater than the input time. Note that an ROI is a series of alternating ROI_USE
  // and ROI_IGNORE values.
  const auto iterUpper = std::upper_bound(m_roi.cbegin(), m_roi.cend(), time);
  if (std::distance(m_roi.cbegin(), iterUpper) % 2 == 0)
    return ROI_IGNORE;
  else
    return ROI_USE;
}

/**
 * Returns the time supplied if it is in a "use" region, or the minimum of the next higher use region.
 * If the ROI is empty, the time is returned.
 * If the Time is after the ROI, an exception is thrown
 * This is intended to be used with logs.
 */
Types::Core::DateAndTime TimeROI::getEffectiveTime(const Types::Core::DateAndTime &time) const {
  if (m_roi.empty()) {
    return time;
  } else if (time > m_roi.back()) {
    throw std::runtime_error("Requesting effective time after the end of the TimeROI");
  } else if (valueAtTime(time) == ROI_USE) {
    return time;
  } else {
    // need to find the start time the first USE region after the time
    auto iter = std::lower_bound(m_roi.begin(), m_roi.end(), time);
    if (valueAtTime(*iter) == ROI_IGNORE) {
      // move to the next value
      iter++;
    }
    return *iter;
  }
}

// returns the first time in the TimeROI
Types::Core::DateAndTime TimeROI::firstTime() const {
  if (m_roi.empty())
    throw std::runtime_error("cannot return time from empty TimeROI");
  return m_roi.front();
}

// returns the last time in the TimeROI
Types::Core::DateAndTime TimeROI::lastTime() const {
  if (m_roi.empty())
    throw std::runtime_error("cannot return time from empty TimeROI");
  return m_roi.back();
}

Types::Core::DateAndTime TimeROI::timeAtIndex(unsigned long index) const {
  return (index < m_roi.size()) ? m_roi[index] : DateAndTime::GPS_EPOCH;
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

  if (roi != nullptr && roi->size() > 0) {
    // make a copy with unique values
    TimeSeriesProperty<bool> roi_copy(*roi);
    roi_copy.eliminateDuplicates(); // takes last value

    // TSP will always give the times sorted in increasing order and the values parallel to that
    const auto values = roi_copy.valuesAsVector();

    // find the first USE value and start there
    const auto iter = std::find(values.cbegin(), values.cend(), ROI_USE);
    if (iter == values.cend())
      throw std::runtime_error("TimeROI cannot be created. All values are ignore.");
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

    // if last value was use, add a new value at the end that is the full duration of the log out
    if (roi->lastValue() == ROI_USE) {
      if (roi->firstTime() == roi->lastTime()) {
        throw std::runtime_error(
            "Cannot guess ending value from a TimeSeriesProperty that contains only a single time");
      } else {
        const auto duration = roi->lastTime() - roi->firstTime();
        m_roi.push_back(roi->lastTime() + duration);
      }
    }
  }

  this->validateValues("TimeROI::replaceROI");
}

void TimeROI::replaceROI(const TimeROI &other) {
  m_roi.clear();
  if (!other.useAll())
    m_roi.assign(other.m_roi.cbegin(), other.m_roi.cend());
}

/// This assumes that the supplied vector is sorted in increasing order of time
void TimeROI::replaceROI(const std::vector<Types::Core::DateAndTime> &roi) {
  m_roi.clear();
  if (!roi.empty())
    m_roi.assign(roi.cbegin(), roi.cend());
}

namespace ROI {
template <typename TYPE>
std::vector<TYPE> calculate_intersection(const std::vector<TYPE> &left, const std::vector<TYPE> &right) {
  // empty is interpreted to mean use everything
  // these checks skip putting together temporary variables for the loops
  if (left == right || right.empty())
    return left;
  else if (left.empty())
    return right;

  // verify that the dimensionality is reasonable
  if (left.size() % 2 != 0)
    throw std::runtime_error("Cannot calculate_intersection with odd left dimension");
  if (right.size() % 2 != 0)
    throw std::runtime_error("Cannot calculate_intersection with odd right dimension");

  // create vector to hold the intersection results
  std::vector<TYPE> result;

  // iterators are faster access than random access via operator[]
  auto it1 = left.cbegin();
  auto it2 = right.cbegin();

  while (it1 != left.cend() && it2 != right.cend()) {
    // Left bound for intersecting segment
    const TYPE &leftBound = std::max(*it1, *it2);

    // Right bound for intersecting segment
    const auto it1_next = std::next(it1);
    const auto it2_next = std::next(it2);
    const TYPE &rightBound = std::min(*it1_next, *it2_next);

    // If segment is valid, include it
    if (leftBound < rightBound) {
      result.emplace_back(leftBound);
      result.emplace_back(rightBound);
    }

    // If it1 right bound is smaller, increment it1; else increment it2
    if (*it1_next < *it2_next)
      std::advance(it1, 2);
    else
      std::advance(it2, 2);
  }

  return result;
}
} // namespace ROI

/**
 * Updates the TimeROI values with the intersection with another TimeROI.
 * See https://en.wikipedia.org/wiki/Intersection for the intersection theory.
 * The algorithm is adapted from https://www.geeksforgeeks.org/find-intersection-of-intervals-given-by-two-lists/
 *
 * Intersection with an empty TimeROI will clear out this one.
 */
void TimeROI::update_intersection(const TimeROI &other) {
  // exit early if the two TimeROI are identical
  if (*this == other)
    return;

  if (other.useAll() || this->useAll()) {
    // empty out this environment
    // this behavior is required by existing calling code due to not universal use of TimeROI
    m_roi.clear();
    return;
  }

  auto output = ROI::calculate_intersection(m_roi, other.m_roi);

  if (output.empty())
    this->replaceROI(USE_NONE);
  else
    m_roi = std::move(output);
}

/**
 * Updates the TimeROI values with the union with another TimeROI.
 * See https://en.wikipedia.org/wiki/Union_(set_theory) for more details
 *
 * Union with an empty TimeROI will do nothing.
 */
void TimeROI::update_union(const TimeROI &other) {
  // exit early if the two TimeROI are identical
  if (*this == other)
    return;

  // add all the intervals from the other
  for (const auto &interval : other.toTimeIntervals()) {
    this->addROI(interval.start(), interval.stop());
  }
}

/**
 * If this is empty, replace it with the supplied TimeROI, otherwise calculate the intersection.
 * Supplying an empty TimeROI will have no effect.
 * Thinking of the TimeROI as filters, the goal is to filter with the intersection or
 * with either of the filters if one of them is empty.
 *
 * @param other :: the replacing or intersecting TimeROI.
 */
void TimeROI::update_or_replace_intersection(const TimeROI &other) {
  if (this->useAll()) {
    this->replaceROI(other);
  } else if (!other.useAll()) {
    this->update_intersection(other);
  }
}

/// This method is to lend itself to helping with transition
const std::vector<Kernel::TimeInterval> TimeROI::toTimeIntervals() const {
  const auto NUM_VAL = m_roi.size();
  std::vector<TimeInterval> output;
  // every other value is a start/stop
  for (std::size_t i = 0; i < NUM_VAL; i += 2) {
    output.emplace_back(m_roi[i], m_roi[i + 1]);
  }

  return output;
}

/**
 * Time intervals returned where no time is before after. This is used in calculating ranges
 * in TimeSeriesProperty.
 * @param after Only give TimeIntervals after this time
 */
const std::vector<Kernel::TimeInterval> TimeROI::toTimeIntervals(const Types::Core::DateAndTime &after) const {
  const auto NUM_VAL = m_roi.size();
  std::vector<TimeInterval> output;
  // every other value is a start/stop
  for (std::size_t i = 0; i < NUM_VAL; i += 2) {
    if (m_roi[i + 1] > after) { // make sure end is after the first time requested
      if (m_roi[i] > after) {   // full region should be used
        output.emplace_back(m_roi[i], m_roi[i + 1]);
      } else {
        output.emplace_back(after, m_roi[i + 1]);
      }
    }
  }

  return output;
}

bool TimeROI::operator==(const TimeROI &other) const { return this->m_roi == other.m_roi; }
bool TimeROI::operator!=(const TimeROI &other) const { return this->m_roi != other.m_roi; }

/**
 * Returns the ROI boundaries to a string.
 * Example:
 *   debugStrPrint(0) returns "0: 2022-Dec-19 00:01:00 to 2022-Dec-26 00:01:00\n" for a single ROI
 *   debugStrPrint(1) returns "2022-Dec-19 00:01:00 2022-Dec-26 00:01:00\n" for a single ROI
 * @param type :: either "0" or "1", for different representation
 * @return ROI boundaries
 */
std::string TimeROI::debugStrPrint(const std::size_t type) const {
  std::stringstream ss;
  if (type == 0) {
    const auto NUM_VALUES{m_roi.size()};
    for (std::size_t i = 0; i < NUM_VALUES; i += 2) {
      ss << (i / 2) << ": " << m_roi[i] << " to " << m_roi[i + 1] << "\n";
    }
  } else if (type == 1) {
    for (const auto &val : m_roi)
      ss << val << " ";
    ss << "\n";
  } else {
    throw std::runtime_error("Invalid type parameter");
  }
  return ss.str();
}

size_t TimeROI::getMemorySize() const { return this->numBoundaries() * sizeof(DateAndTime); }

/**
 * Duration of the whole TimeROI
 */
double TimeROI::durationInSeconds() const {
  const auto ROI_SIZE = this->numBoundaries();
  if (ROI_SIZE == 0) {
    return 0.;
  } else if (this->useNone()) {
    return -1.;
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
  if (this->useAll()) {
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
    std::stringstream msg;
    msg << "In " << label << ": Values are not unique";
    throw std::runtime_error(msg.str());
  }
}

std::size_t TimeROI::numBoundaries() const { return static_cast<std::size_t>(m_roi.size()); }

std::size_t TimeROI::numberOfRegions() const { return this->numBoundaries() / 2; }

bool TimeROI::empty() const { return bool(this->numBoundaries() == 0); }

bool TimeROI::useAll() const { return this->empty(); }

bool TimeROI::useNone() const { return *this == USE_NONE; }

/// Removes all ROI's, leaving an empty object
void TimeROI::clear() { m_roi.clear(); }

// serialization / deserialization items
void TimeROI::saveNexus(::NeXus::File *file) const {
  // create a local TimeSeriesProperty which will do the actual work
  TimeSeriesProperty<bool> tsp(NAME);
  for (const auto &interval : this->toTimeIntervals()) {
    tsp.addValue(interval.start(), ROI_USE);
    tsp.addValue(interval.stop(), ROI_IGNORE);
  }

  // save things to to disk
  tsp.saveProperty(file);
}

namespace ROI {
// concrete instantiations need to be exported
template MANTID_KERNEL_DLL std::vector<std::size_t> calculate_intersection(const std::vector<std::size_t> &left,
                                                                           const std::vector<std::size_t> &right);
template MANTID_KERNEL_DLL std::vector<Types::Core::DateAndTime>
calculate_intersection(const std::vector<Types::Core::DateAndTime> &left,
                       const std::vector<Types::Core::DateAndTime> &right);
} // namespace ROI

} // namespace Kernel
} // namespace Mantid
