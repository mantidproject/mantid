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
/// the underlying property needs a name
const std::string NAME{"Kernel_TimeROI"};

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

TimeROI::TimeROI() : m_roi{NAME} {}

TimeROI::TimeROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) : m_roi{NAME} {
  this->addROI(startTime, stopTime);
}

void TimeROI::addROI(const std::string &startTime, const std::string &stopTime) {
  this->addROI(DateAndTime(startTime), DateAndTime(stopTime));
}

void TimeROI::addROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  assert_increasing(startTime, stopTime);
  m_roi.addValue(startTime, ROI_USE);
  m_roi.addValue(stopTime, ROI_IGNORE);
}

void TimeROI::addROI(const std::time_t &startTime, const std::time_t &stopTime) {
  this->addROI(DateAndTime(startTime), DateAndTime(stopTime));
}

void TimeROI::addMask(const std::string &startTime, const std::string &stopTime) {
  this->addMask(DateAndTime(startTime), DateAndTime(stopTime));
}

void TimeROI::addMask(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  assert_increasing(startTime, stopTime);
  m_roi.addValue(startTime, ROI_IGNORE);
  m_roi.addValue(stopTime, ROI_USE);
}

void TimeROI::addMask(const std::time_t &startTime, const std::time_t &stopTime) {
  this->addMask(DateAndTime(startTime), DateAndTime(stopTime));
}

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
  bool set_values = std::any_of(values.cbegin(), values.cend(), [](bool value) { return value; });

  // set the values if there are any use regions
  if (set_values) {
    this->m_roi.addValues(times, values);
  }
}

/**
 * Updates the TimeROI values with the union with another TimeROI.
 * See https://en.wikipedia.org/wiki/Union_(set_theory) for more details
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

void TimeROI::removeRedundantEntries() {
  if (this->numBoundaries() < 2) {
    return; // nothing to do with zero or one elements
  }

  // when an individual time has multiple values, use the last value added
  m_roi.eliminateDuplicates();

  // get a copy of the current roi
  const auto values_old = m_roi.valuesAsVector();
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

bool TimeROI::operator==(const TimeROI &other) const { return this->m_roi == other.m_roi; }

void TimeROI::debugPrint() const {
  const auto values = m_roi.valuesAsVector();
  const auto times = m_roi.timesAsVector();
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << i << ": " << times[i] << ", " << values[i] << std::endl;
  }
}

/**
 * duration of the whole TimeROI
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
 * duration of the TimeROI between startTime and stopTime
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

} // namespace Kernel
} // namespace Mantid
