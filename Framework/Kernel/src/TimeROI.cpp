// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include <iostream> // TODO REMOVE
#include <limits>

#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeROI.h"

namespace Mantid {
namespace Kernel {

using Mantid::Types::Core::DateAndTime;

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

/**
 * This returns true if the first value is USE, the last value is IGNORE, and all the others alternate.
 * The assumption is that if the values meet this criteria, there is no reason to try to reduce them because they are
 * already unique and minimal number of values.
 */
bool valuesAreAlternating(const std::vector<bool> &values) {
  // empty object is fine
  if (values.empty())
    return true;

  const auto NUM_VALUES = values.size();
  // should be an even number of values
  if (NUM_VALUES % 2 != 0)
    return false;

  // the values must start with use and end with ignore
  if ((values.front() == ROI_IGNORE) || (values.back() == ROI_USE))
    return false;

  // even entries should be use and odd should be ignore
  for (size_t i = 0; i < NUM_VALUES; ++i) {
    if (i % 2 == 0) {
      if (values[i] == ROI_IGNORE) // even entries should be USE
        return false;
    } else {
      if (values[i] == ROI_USE) { // odd entries should be IGNORE
        return false;
      }
    }
  }
  return true;
}
} // namespace

const std::string TimeROI::NAME = "Kernel_TimeROI";

TimeROI::TimeROI() : m_roi{NAME} {}

TimeROI::TimeROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) : m_roi{NAME} {
  this->addROI(startTime, stopTime);
}

TimeROI::TimeROI(const Kernel::TimeSeriesProperty<bool> &filter) : m_roi{NAME} {
  const auto &values = filter.valuesAsVector();

  // only need to do something if values aren't all USE
  if (!std::all_of(values.cbegin(), values.cend(), [](const bool value) { return value == ROI_USE; })) {
    const auto &times = filter.timesAsVector();
    const auto NUM_VAL = times.size();
    for (size_t i = 0; i < NUM_VAL; ++i) {
      m_roi.addValue(times[i], values[i]);
    }

    // assuming the filter was not well specified, clean things up
    this->removeRedundantEntries();
  }
}

void TimeROI::addROI(const std::string &startTime, const std::string &stopTime) {
  this->addROI(DateAndTime(startTime), DateAndTime(stopTime));
}

/**
 * Add new region as a union.
 */
void TimeROI::addROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  assert_increasing(startTime, stopTime);
  if ((this->empty()) || (startTime > m_roi.lastTime()) || (stopTime < m_roi.firstTime())) {
    // add in the new region
    m_roi.addValue(startTime, ROI_USE);
    m_roi.addValue(stopTime, ROI_IGNORE);
  } else if (this->isCompletelyInROI(startTime, stopTime)) {
    g_log.debug("TimeROI::addROI is already accounted for and being ignored");
  } else {
    g_log.debug("TimeROI::addROI using union method");
    // add as an union with this
    this->update_union(TimeROI(startTime, stopTime));
  }
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
  } else if ((startTime > m_roi.lastTime()) || (stopTime < m_roi.firstTime())) {
    g_log.debug("TimeROI::addMask to ignored region");
  } else if ((startTime <= m_roi.firstTime()) && (stopTime >= m_roi.lastTime())) {
    // the mask includes everything so remove all current values
    this->m_roi.clear();
  } else if (isCompletelyInROI(startTime, stopTime)) {
    g_log.debug("TimeROI::addMask cutting notch in existing ROI");
    // cutting a notch in an existing ROI
    m_roi.addValue(startTime, ROI_IGNORE);
    m_roi.addValue(stopTime, ROI_USE);
  } else {
    g_log.debug("TimeROI::addMask using intersection method");
    // create an ROI that is full possible range minus the mask then intersect it
    TimeROI temp(std::min(m_roi.firstTime(), startTime), std::max(m_roi.lastTime(), stopTime));
    temp.m_roi.addValue(startTime, ROI_IGNORE);
    temp.m_roi.addValue(stopTime, ROI_USE);

    this->update_intersection(temp);
  }
}

void TimeROI::addMask(const std::time_t &startTime, const std::time_t &stopTime) {
  this->addMask(DateAndTime(startTime), DateAndTime(stopTime));
}

/**
 * This method returns true if the entire region between startTime and stopTime is inside an existing ROI.
 * If part of the supplied region is not covered this returns false.
 */
bool TimeROI::isCompletelyInROI(const Types::Core::DateAndTime &startTime,
                                const Types::Core::DateAndTime &stopTime) const {
  // check if the region is in the overall window at all
  if ((startTime > m_roi.lastTime()) || (stopTime < m_roi.firstTime()))
    return false;

  // since the ROI should be alternating "use" and "ignore", see if the start and stop are within a single region
  const auto &times = m_roi.timesAsVector();
  const auto iterStart = std::lower_bound(times.cbegin(), times.cend(), startTime);
  const auto iterStop = std::lower_bound(iterStart, times.cend(), stopTime);
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
  if (this->empty() || time < m_roi.firstTime()) {
    return ROI_IGNORE;
  } else {
    return m_roi.getSingleValue(time);
  }
}

/// get a list of all unique times. order is not guaranteed
std::vector<DateAndTime> TimeROI::getAllTimes(const TimeROI &other) {

  std::set<DateAndTime> times_set;
  const auto times_lft = this->m_roi.timesAsVector();
  for (const auto time : times_lft)
    times_set.insert(time);
  const auto times_rgt = other.m_roi.timesAsVector();
  for (const auto time : times_rgt)
    times_set.insert(time);

  // copy into the vector
  std::vector<DateAndTime> times_all;
  times_all.assign(times_set.begin(), times_set.end());

  return times_all;
}

void TimeROI::replaceValues(const std::vector<DateAndTime> &times, const std::vector<bool> &values) {
  if (times.size() != values.size()) {
    std::stringstream msg;
    msg << "Times and Values are different size: " << times.size() << " != " << values.size();
    throw std::runtime_error(msg.str());
  }

  // remove all current values
  this->m_roi.clear();

  // see if everything to add is "IGNORE"
  bool set_values = std::any_of(values.cbegin(), values.cend(), [](const bool value) { return value == ROI_USE; });

  // set the values if there are any use regions
  if (set_values) {
    this->m_roi.addValues(times, values);
  }
}

void TimeROI::replaceROI(const TimeSeriesProperty<bool> *roi) {
  // this is used by LogManager::loadNexus
  const auto times = roi->timesAsVector();
  const auto values = roi->valuesAsVector();
  this->replaceValues(times, values);
}

void TimeROI::replaceROI(const TimeROI &other) {
  const auto times = other.m_roi.timesAsVector();
  const auto values = other.m_roi.valuesAsVector();
  this->replaceValues(times, values);
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

  // get rid of redundant entries before starting
  this->removeRedundantEntries();
  // get a list of all unique times
  std::vector<DateAndTime> times_all = getAllTimes(other);

  // calculate what values to add
  std::vector<bool> additional_values(times_all.size());
  std::transform(times_all.begin(), times_all.end(), additional_values.begin(), [this, other](const DateAndTime &time) {
    return bool(this->valueAtTime(time) || other.valueAtTime(time));
  });

  // remove old values and replace with new ones
  this->replaceValues(times_all, additional_values);
  this->removeRedundantEntries();
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

  // get rid of redundant entries before starting
  this->removeRedundantEntries();

  // get a list of all unique times
  std::vector<DateAndTime> times_all = getAllTimes(other);

  // calculate what values to add
  std::vector<bool> additional_values(times_all.size());
  std::transform(times_all.begin(), times_all.end(), additional_values.begin(), [this, other](const DateAndTime &time) {
    return bool(this->valueAtTime(time) && other.valueAtTime(time));
  });

  // remove old values and replace with new ones
  this->replaceValues(times_all, additional_values);
  this->removeRedundantEntries();
}

/**
 * Remove time/value pairs that are not necessary to describe the TimeROI
 * - Sort the times/values
 * - Ensure that values start with USE and should end with IGNORE
 * - Remove values that are not needed (e.g. IGNORE followed by IGNORE)
 * - Remove values that are overridden. Overridden values are ones where a new value was added at the same time, the
 * last one added will be used.
 */
void TimeROI::removeRedundantEntries() {
  if (this->numBoundaries() < 2) {
    return; // nothing to do with zero or one elements
  }

  // when an individual time has multiple values, use the last value added
  m_roi.eliminateDuplicates();

  // get a copy of the current roi
  const auto values_old = m_roi.valuesAsVector();
  if (valuesAreAlternating(values_old)) {
    // there is nothing more to do
    return;
  }
  const auto times_old = m_roi.timesAsVector();
  const auto ORIG_SIZE = values_old.size();

  // create new vector to put result into
  std::vector<bool> values_new;
  std::vector<DateAndTime> times_new;

  // skip ahead to first time that isn't ignore
  // since before being in the ROI means ignore
  std::size_t index_old = 0;
  while (values_old[index_old] == ROI_IGNORE) {
    index_old++;
  }
  // add the current location which will always start with use
  values_new.push_back(ROI_USE);
  times_new.push_back(times_old[index_old]);
  index_old++; // advance past location just added

  // copy in values that aren't the same as the ones before them
  for (; index_old < ORIG_SIZE; ++index_old) {
    if (values_old[index_old] != values_old[index_old - 1]) {
      values_new.push_back(values_old[index_old]);
      times_new.push_back(times_old[index_old]);
    }
  }

  // update the member value if anything has changed
  if (values_new.size() != ORIG_SIZE)
    m_roi.replaceValues(times_new, values_new);
}

/**
 * This method is to lend itself to be compatible with existing implementation
 */
const Kernel::SplittingIntervalVec TimeROI::toSplitters() const {
  Kernel::SplittingIntervalVec output;

  if (!(this->empty())) {
    // since this is const, complain if there is something not right with assumptions
    if (!(valuesAreAlternating(m_roi.valuesAsVector()))) {
      throw std::runtime_error(
          "Must call TimeROI::removeRedundantEntries() before using TimeROI::toSplitters() to have "
          "minimal number of splitters");
    }

    // and the current times
    const auto times = m_roi.timesAsVector();
    const auto NUM_TIMES = times.size();

    // convert to a vector of splitters
    for (size_t i = 0; i < NUM_TIMES - 1; i += 2)
      output.push_back(SplittingInterval(times[i], times[i + 1]));
  }

  return output;
}

bool TimeROI::operator==(const TimeROI &other) const { return this->m_roi == other.m_roi; }

void TimeROI::debugPrint() const {
  const auto values = m_roi.valuesAsVector();
  const auto times = m_roi.timesAsVector();
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << i << ": " << times[i] << ", " << values[i] << std::endl;
  }
}

size_t TimeROI::getMemorySize() const { return m_roi.getMemorySize(); }

/**
 * Duration of the whole TimeROI
 */
double TimeROI::durationInSeconds() const {
  const auto ROI_SIZE = this->numBoundaries();
  if (ROI_SIZE == 0) {
    return 0.;
  } else if (m_roi.lastValue() == ROI_USE) {
    return std::numeric_limits<double>::infinity();
  } else {
    const std::vector<bool> &values = m_roi.valuesAsVector();
    const std::vector<double> &times = m_roi.timesAsVectorSeconds();
    double total{0.};
    for (std::size_t i = 0; i < ROI_SIZE - 1; ++i) {
      if (values[i])
        total += (times[i + 1] - times[i]);
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
  if (stopTime <= m_roi.firstTime()) { // asking before ROI
    return 0.;
  } else if (startTime >= m_roi.lastTime()) { // asking after ROI
    return 0.;
  } else if ((startTime <= m_roi.firstTime()) && (stopTime >= m_roi.lastTime())) { // full range of ROI
    return this->durationInSeconds();
  } else { // do the calculation
    // the time requested is an intersection of start/stop time and this object
    TimeROI temp{startTime, stopTime};
    temp.update_intersection(*this);
    return temp.durationInSeconds();
  }
}

std::size_t TimeROI::numBoundaries() const { return static_cast<std::size_t>(m_roi.size()); }

bool TimeROI::empty() const { return bool(this->numBoundaries() == 0); }

// serialization / deserialization items
void TimeROI::saveNexus(::NeXus::File *file) const { const_cast<TimeSeriesProperty<bool> &>(m_roi).saveProperty(file); }

} // namespace Kernel
} // namespace Mantid
