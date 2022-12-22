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

using Types::Core::DateAndTime;

namespace {
/// static Logger definition
Logger g_log("TimeROI");
/// the underlying property needs a name
const std::string NAME{"Kernel_TimeROI"};

const bool ROI_USE{true};
const bool ROI_IGNORE{false};

} // namespace

TimeROI::TimeROI() : m_roi{NAME} {}

void TimeROI::addROI(const std::string &startTime, const std::string &stopTime) {
  m_roi.addValue(startTime, ROI_USE);
  m_roi.addValue(stopTime, ROI_IGNORE);
}

void TimeROI::addROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  m_roi.addValue(startTime, ROI_USE);
  m_roi.addValue(stopTime, ROI_IGNORE);
}

void TimeROI::addROI(const std::time_t &startTime, const std::time_t &stopTime) {
  m_roi.addValue(startTime, ROI_USE);
  m_roi.addValue(stopTime, ROI_IGNORE);
}

void TimeROI::addMask(const std::string &startTime, const std::string &stopTime) {
  m_roi.addValue(startTime, ROI_IGNORE);
  m_roi.addValue(stopTime, ROI_USE);
}

void TimeROI::addMask(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) {
  m_roi.addValue(startTime, ROI_IGNORE);
  m_roi.addValue(stopTime, ROI_USE);
}

void TimeROI::addMask(const std::time_t &startTime, const std::time_t &stopTime) {
  m_roi.addValue(startTime, ROI_IGNORE);
  m_roi.addValue(stopTime, ROI_USE);
}

void TimeROI::removeRedundantEntries() {
  if (this->empty()) {
    return; // nothing to do with empty
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

void TimeROI::debugPrint() const {
  const auto values = m_roi.valuesAsVector();
  const auto times = m_roi.timesAsVector();
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << i << ": " << times[i] << ", " << values[i] << std::endl;
  }
}

double TimeROI::durationInSeconds() const {
  const auto ROI_SIZE = this->numBoundaries();
  if (ROI_SIZE == 0) {
    return 0.;
  } else if (m_roi.lastValue() == ROI_USE) {
    return std::numeric_limits<double>::infinity();
  } else {
    const std::vector<bool> &values = m_roi.valuesAsVector();
    const std::vector<double> &times = m_roi.timesAsVectorSeconds();
    double total = 0.;
    for (std::size_t i = 0; i < ROI_SIZE - 1; ++i) {
      if (values[i])
        total += (times[i + 1] - times[i]);
    }

    return total;
  }
}

std::size_t TimeROI::numBoundaries() const { return static_cast<std::size_t>(m_roi.size()); }

bool TimeROI::empty() const { return bool(this->numBoundaries() == 0); }

} // namespace Kernel
} // namespace Mantid
