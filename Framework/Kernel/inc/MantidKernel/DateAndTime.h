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
  TimeInterval() : m_begin(), m_end() {}
  /// Constructor
  TimeInterval(const Types::Core::DateAndTime &from, const Types::Core::DateAndTime &to);
  /// Beginning of the interval
  Types::Core::DateAndTime begin() const { return m_begin; }
  /// End of the interval
  Types::Core::DateAndTime end() const { return m_end; }
  /// True if the interval is not empty
  bool isValid() const { return m_end > m_begin; }

  /// Interval length (in seconds?)
  Types::Core::time_duration length() const { return m_end - m_begin; }

  /// True if the interval contains \a t.
  bool contains(const Types::Core::DateAndTime &t) const { return t >= begin() && t < end(); }
  /// Returns an intersection of two intervals
  TimeInterval intersection(const TimeInterval &ti) const;
  /// Returns true if this interval ends before \a ti starts
  bool operator<(const TimeInterval &ti) const { return end() < ti.begin(); }
  /// String representation of the begin time
  std::string begin_str() const;
  /// String representation of the end time
  std::string end_str() const;

  /** Stream output operator  */
  friend MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &s, const Mantid::Kernel::TimeInterval &t);

private:
  /// begin
  Types::Core::DateAndTime m_begin;
  /// end
  Types::Core::DateAndTime m_end;
};

} // namespace Kernel
} // namespace Mantid
