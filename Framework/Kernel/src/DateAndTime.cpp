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

TimeInterval::TimeInterval(const Types::Core::DateAndTime &from, const Types::Core::DateAndTime &to) : m_begin(from) {
  if (to > from)
    m_end = to;
  else
    m_end = from;
}

TimeInterval::TimeInterval(const std::string &from, const std::string &to) {
  const DateAndTime fromObj(from);
  const DateAndTime toObj(to);

  m_begin = fromObj;
  if (toObj > fromObj)
    m_end = toObj;
  else
    m_end = fromObj;
}

bool TimeInterval::overlaps(const TimeInterval *other) const {
  const auto thisBegin = this->begin();
  const auto thisEnd = this->end();
  const auto otherBegin = other->begin();
  const auto otherEnd = other->end();

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

  const auto t1 = std::max(begin(), ti.begin());
  const auto t2 = std::min(end(), ti.end());

  return t1 < t2 ? TimeInterval(t1, t2) : TimeInterval();
}

bool TimeInterval::operator<(const TimeInterval &ti) const {
  if (end() < ti.begin())
    return true;
  else if (end() == ti.begin())
    return begin() < ti.begin();
  else
    return false;
}
bool TimeInterval::operator>(const TimeInterval &ti) const {
  if (begin() > ti.end())
    return true;
  else if (begin() == ti.end())
    return begin() > ti.begin();
  else
    return false;
}

bool TimeInterval::operator==(const TimeInterval &ti) const { return (begin() == ti.begin()) && (end() == ti.end()); }

/// String representation of the begin time
std::string TimeInterval::begin_str() const { return boost::posix_time::to_simple_string(this->m_begin.to_ptime()); }

/// String representation of the end time
std::string TimeInterval::end_str() const { return boost::posix_time::to_simple_string(this->m_end.to_ptime()); }

std::ostream &operator<<(std::ostream &s, const Mantid::Kernel::TimeInterval &t) {
  s << t.begin().toSimpleString() << " - " << t.end().toSimpleString();
  return s;
}

} // namespace Kernel

} // namespace Mantid
