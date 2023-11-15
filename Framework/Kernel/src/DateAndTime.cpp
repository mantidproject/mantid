// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DateAndTime.h"

#include <boost/date_time/date.hpp>
#include <boost/date_time/time.hpp>

#include <cmath>
#include <exception>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>

namespace Mantid {

using namespace Types::Core;
namespace Kernel {

TimeInterval::TimeInterval(const Types::Core::DateAndTime &from, const Types::Core::DateAndTime &to) : m_start(from) {
  if (to > from)
    m_stop = to;
  else
    m_stop = from;
}

TimeInterval::TimeInterval(const std::string &from, const std::string &to) {
  const DateAndTime fromObj(from);
  const DateAndTime toObj(to);

  m_start = fromObj;
  if (toObj > fromObj)
    m_stop = toObj;
  else
    m_stop = fromObj;
}

bool TimeInterval::overlaps(const TimeInterval *other) const {
  const auto &thisBegin = this->start();
  const auto &thisEnd = this->stop();
  const auto &otherBegin = other->start();
  const auto &otherEnd = other->stop();

  return ((otherBegin < thisEnd) && (otherBegin >= thisBegin)) || ((otherEnd < thisEnd) && (otherEnd >= thisBegin)) ||
         ((thisBegin < otherEnd) && (thisBegin >= otherBegin)) || ((thisEnd < otherEnd) && (thisEnd >= otherBegin));
}

bool TimeInterval::overlaps(const TimeInterval &other) const { return this->overlaps(&other); }

/**  Returns an intersection of this interval with \a ti
     @param ti :: Time interval
     @return A valid time interval if this interval intersects with \a ti or
             an empty interval otherwise.
 */
TimeInterval TimeInterval::intersection(const TimeInterval &ti) const {
  if (!isValid() || !ti.isValid() || !this->overlaps(&ti))
    return TimeInterval();

  const auto t1 = std::max(start(), ti.start());
  const auto t2 = std::min(stop(), ti.stop());

  return t1 < t2 ? TimeInterval(t1, t2) : TimeInterval();
}

bool TimeInterval::operator<(const TimeInterval &ti) const {
  if (stop() < ti.start())
    return true;
  else if (stop() == ti.start())
    return start() < ti.start();
  else
    return false;
}
bool TimeInterval::operator>(const TimeInterval &ti) const {
  if (start() > ti.stop())
    return true;
  else if (start() == ti.stop())
    return start() > ti.start();
  else
    return false;
}

Types::Core::time_duration TimeInterval::length() const { return m_stop - m_start; }

double TimeInterval::duration() const { return Types::Core::DateAndTime::secondsFromDuration(this->length()); }

bool TimeInterval::operator==(const TimeInterval &ti) const { return (start() == ti.start()) && (stop() == ti.stop()); }

/// String representation of the begin time
std::string TimeInterval::begin_str() const { return boost::posix_time::to_simple_string(this->m_start.to_ptime()); }

/// String representation of the end time
std::string TimeInterval::end_str() const { return boost::posix_time::to_simple_string(this->m_stop.to_ptime()); }

std::ostream &operator<<(std::ostream &s, const Mantid::Kernel::TimeInterval &t) {
  s << t.start().toSimpleString() << " - " << t.stop().toSimpleString();
  return s;
}

} // namespace Kernel

} // namespace Mantid
