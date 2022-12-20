// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include <iostream>

#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeROI.h"

namespace Mantid {
namespace Kernel {

using Types::Core::DateAndTime;

namespace {
/// static Logger definition
Logger g_log("TimeROI");
/// the underlying property needs a name
const std::string NAME{"roi"};

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

double TimeROI::durationInSeconds() const {
  const std::size_t ROI_SIZE = static_cast<std::size_t>(m_roi.size());
  if (ROI_SIZE == 0) {
    return 0.;
  } else if (ROI_SIZE == 1) {
    throw std::runtime_error("TimeROI with only 1 value should not be possible");
    // TODO should be impossible throw an exception
  } else if (ROI_SIZE == 2) {
    const auto &startTime = m_roi.firstTime();
    const auto &stopTime = m_roi.lastTime();
    return DateAndTime::secondsFromDuration(stopTime - startTime);
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

} // namespace Kernel
} // namespace Mantid
