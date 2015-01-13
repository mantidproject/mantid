#ifndef DATE_AND_TIME_H
#define DATE_AND_TIME_H

#include "MantidKernel/DllConfig.h"
#ifndef Q_MOC_RUN
#include <boost/date_time/posix_time/posix_time.hpp>
#endif
#include <iosfwd>

namespace Mantid {
namespace Kernel {

/// Durations and time intervals
typedef boost::posix_time::time_duration time_duration;

//=============================================================================================
/** Class for holding the date and time in Mantid.
 * It is stored as a signed 64-bit int of the # of nanoseconds since Jan 1,
 *1990.
 * This allows nano-second resolution time while allowing +- 292 years around
 *1990.
 * (boost::posix_time at nanosecond resolution uses 96 bits).
 *
 * @author Janik Zikovsky, SNS
 * @date November 12, 2010
 *
 *
 * */

// Make the compiler pack the data size aligned to 1-byte, to use as little
// space as possible
#pragma pack(push, 1)
class MANTID_KERNEL_DLL DateAndTime {
public:
  DateAndTime();
  DateAndTime(const int64_t total_nanoseconds);
  DateAndTime(const double seconds, const double nanoseconds);
  DateAndTime(const int32_t seconds, const int32_t nanoseconds);
  DateAndTime(const int64_t seconds, const int64_t nanoseconds);
  DateAndTime(const std::string ISO8601_string);
  DateAndTime(const boost::posix_time::ptime _ptime);

  void set_from_ptime(boost::posix_time::ptime _ptime);
  boost::posix_time::ptime to_ptime() const;

  void set_from_time_t(std::time_t _timet);
  std::time_t to_time_t() const;
  std::tm to_localtime_tm() const;
  std::time_t to_localtime_t() const;
  std::tm to_tm() const;

  void setFromISO8601(const std::string str);
  std::string toSimpleString() const;
  std::string
  toFormattedString(const std::string format = "%Y-%b-%d %H:%M:%S") const;
  std::string toISO8601String() const;

  /// Stream output operator
  friend MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &stream,
                                                    const DateAndTime &t);

  void setToMaximum();
  void setToMinimum();

  int year() const;
  int month() const;
  int day() const;
  int hour() const;
  int minute() const;
  int second() const;
  int nanoseconds() const;
  int64_t totalNanoseconds() const;

  bool operator==(const DateAndTime &rhs) const;
  bool operator==(const boost::posix_time::ptime &rhs) const;
  bool operator!=(const DateAndTime &rhs) const;
  bool operator<(const DateAndTime &rhs) const;
  bool operator<=(const DateAndTime &rhs) const;
  bool operator>(const DateAndTime &rhs) const;
  bool operator>=(const DateAndTime &rhs) const;
  bool equals(const DateAndTime &rhs, const int64_t tol = 1) const;

  DateAndTime operator+(const int64_t nanosec) const;
  DateAndTime &operator+=(const int64_t nanosec);
  DateAndTime operator-(const int64_t nanosec) const;
  DateAndTime &operator-=(const int64_t nanosec);

  DateAndTime operator+(const time_duration &td) const;
  DateAndTime &operator+=(const time_duration &td);
  DateAndTime operator-(const time_duration &td) const;
  DateAndTime &operator-=(const time_duration &td);

  DateAndTime operator+(const double seconds) const;
  DateAndTime &operator+=(const double seconds);
  DateAndTime operator-(const double seconds) const;
  DateAndTime &operator-=(const double seconds);

  time_duration operator-(const DateAndTime &rhs) const;

  //-------------- STATIC FUNCTIONS -----------------------
  static DateAndTime getCurrentTime();
  static DateAndTime maximum();
  static DateAndTime minimum();
  static double secondsFromDuration(time_duration td);
  static time_duration durationFromSeconds(double seconds);
  static int64_t nanosecondsFromDuration(const time_duration &td);
  static int64_t nanosecondsFromSeconds(double sec);
  static time_duration durationFromNanoseconds(int64_t dur);
  static const DateAndTime defaultTime();
  static void createVector(const DateAndTime start,
                           const std::vector<double> &seconds,
                           std::vector<DateAndTime> &out);
  static bool stringIsISO8601(const std::string &str);

private:
  /// A signed 64-bit int of the # of nanoseconds since Jan 1, 1990.
  int64_t _nanoseconds;
};
#pragma pack(pop)

namespace DateAndTimeHelpers {

/// The difference in seconds between standard unix and gps epochs.
static const uint32_t EPOCH_DIFF = 631152000;

/// The epoch for GPS times.
static const boost::posix_time::ptime GPS_EPOCH(boost::gregorian::date(1990, 1,
                                                                       1));

/// Const of one second time duration
static const time_duration oneSecond =
    boost::posix_time::time_duration(0, 0, 1, 0);

MANTID_KERNEL_DLL time_t utc_mktime(struct tm *utctime);
}

/** Represents a time interval.

    @author Roman Tolchenov, Tessella plc,
    @date 25/03/2009

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL TimeInterval {
public:
  /// Default constructor
  TimeInterval() : m_begin(), m_end() {}
  /// Constructor
  TimeInterval(const DateAndTime &from, const DateAndTime &to);
  /// Beginning of the interval
  DateAndTime begin() const { return m_begin; }
  /// End of the interval
  DateAndTime end() const { return m_end; }
  /// True if the interval is not empty
  bool isValid() const { return m_end > m_begin; }

  /// Interval length (in seconds?)
  time_duration length() const { return m_end - m_begin; }

  /// True if the interval contains \a t.
  bool contains(const DateAndTime &t) const {
    return t >= begin() && t < end();
  }
  /// Returns an intersection of two intervals
  TimeInterval intersection(const TimeInterval &ti) const;
  /// Returns true if this interval ends before \a ti starts
  bool operator<(const TimeInterval &ti) const { return end() < ti.begin(); }
  /// String representation of the begin time
  std::string begin_str() const;
  /// String representation of the end time
  std::string end_str() const;

  /** Stream output operator  */
  friend MANTID_KERNEL_DLL std::ostream &
  operator<<(std::ostream &s, const Mantid::Kernel::TimeInterval &t);

private:
  /// begin
  DateAndTime m_begin;
  /// end
  DateAndTime m_end;
};

} // namespace Kernel
} // namespace Mantid

#endif // DATE_AND_TIME_H
