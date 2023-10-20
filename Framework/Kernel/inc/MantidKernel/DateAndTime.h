// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <cstdint>
#include <ctime>
#include <iosfwd>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** Represents a time interval.

    @author Roman Tolchenov, Tessella plc,
    @date 25/03/2009
*/
class MANTID_KERNEL_DLL TimeInterval {
public:
  /// Default constructor
  TimeInterval() : m_start(), m_stop() {}
  /// Constructor
  TimeInterval(const Types::Core::DateAndTime &from, const Types::Core::DateAndTime &to);
  TimeInterval(const std::string &from, const std::string &to);

  /// Beginning of the interval
  const Types::Core::DateAndTime &start() const { return m_start; }
  /// End of the interval
  const Types::Core::DateAndTime &stop() const { return m_stop; }
  /// True if the interval is not empty
  bool isValid() const { return m_stop > m_start; }

  /// Interval length (in seconds?)
  Types::Core::time_duration length() const;
  /// in seconds
  double duration() const;

  /// True if the interval contains \a t.
  bool contains(const Types::Core::DateAndTime &t) const { return t >= start() && t < stop(); }
  /// Return true if the SplittingInterval overlaps with this one.
  bool overlaps(const TimeInterval *other) const;
  /// Return true if the SplittingInterval overlaps with this one.
  bool overlaps(const TimeInterval &other) const;

  /// Returns an intersection of two intervals
  TimeInterval intersection(const TimeInterval &ti) const;
  /// Returns true if this interval ends before \a ti starts
  bool operator<(const TimeInterval &ti) const;
  bool operator>(const TimeInterval &ti) const;
  bool operator==(const TimeInterval &ti) const;
  /// String representation of the begin time
  std::string begin_str() const;
  /// String representation of the end time
  std::string end_str() const;

  /** Stream output operator  */
  friend MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &s, const Mantid::Kernel::TimeInterval &t);

private:
  /// begin
  Types::Core::DateAndTime m_start;
  /// end
  Types::Core::DateAndTime m_stop;
};

} // namespace Kernel
} // namespace Mantid
