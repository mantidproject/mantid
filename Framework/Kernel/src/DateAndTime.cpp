// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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

TimeInterval::TimeInterval(const Types::Core::DateAndTime &from,
                           const Types::Core::DateAndTime &to)
    : m_begin(from) {
  if (to > from)
    m_end = to;
  else
    m_end = from;
}

/**  Returns an intersection of this interval with \a ti
     @param ti :: Time interval
     @return A valid time interval if this interval intersects with \a ti or
             an empty interval otherwise.
 */
TimeInterval TimeInterval::intersection(const TimeInterval &ti) const {
  if (!isValid() || !ti.isValid())
    return TimeInterval();

  DateAndTime t1 = begin();
  if (ti.begin() > t1)
    t1 = ti.begin();

  DateAndTime t2 = end();
  if (ti.end() < t2)
    t2 = ti.end();

  return t1 < t2 ? TimeInterval(t1, t2) : TimeInterval();
}

/// String representation of the begin time
std::string TimeInterval::begin_str() const {
  return boost::posix_time::to_simple_string(this->m_begin.to_ptime());
}

/// String representation of the end time
std::string TimeInterval::end_str() const {
  return boost::posix_time::to_simple_string(this->m_end.to_ptime());
}

std::ostream &operator<<(std::ostream &s,
                         const Mantid::Kernel::TimeInterval &t) {
  s << t.begin().toSimpleString() << " - " << t.end().toSimpleString();
  return s;
}

} // namespace Kernel

} // namespace Mantid
