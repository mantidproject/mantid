// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidTypes/DllConfig.h"
#ifndef Q_MOC_RUN
#include <boost/date_time/posix_time/posix_time.hpp>
#endif
#include <cstdint>
#include <ctime>
#include <iosfwd>
#include <string>
#include <vector>

namespace Mantid {
namespace Types {
namespace Core {
/// Durations and time intervals
using time_duration = boost::posix_time::time_duration;

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
 */

// Make the compiler pack the data size aligned to 1-byte, to use as little
// space as possible
class MANTID_TYPES_DLL DateAndTime {
public:
  DateAndTime();
  DateAndTime(const int64_t total_nanoseconds);
  DateAndTime(const double seconds, const double nanoseconds);
  DateAndTime(const int32_t seconds, const int32_t nanoseconds);
  DateAndTime(const int64_t seconds, const int64_t nanoseconds);
  DateAndTime(const std::string &ISO8601_string);
  DateAndTime(const boost::posix_time::ptime &_ptime);

  void set_from_ptime(const boost::posix_time::ptime &_ptime);
  boost::posix_time::ptime to_ptime() const;

  void set_from_time_t(std::time_t _timet);
  std::time_t to_time_t() const;
  std::tm to_localtime_tm() const;
  std::time_t to_localtime_t() const;
  std::tm to_tm() const;

  void setFromISO8601(const std::string &str);
  std::string toSimpleString() const;
  std::string toFormattedString(const std::string &format = "%Y-%b-%d %H:%M:%S") const;
  std::string toISO8601String() const;

  /// Stream output operator
  friend MANTID_TYPES_DLL std::ostream &operator<<(std::ostream &stream, const DateAndTime &t);

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

  inline bool operator==(const DateAndTime &rhs) const { return _nanoseconds == rhs._nanoseconds; }
  bool operator==(const boost::posix_time::ptime &rhs) const;
  bool operator!=(const DateAndTime &rhs) const;
  inline bool operator<(const DateAndTime &rhs) const { return _nanoseconds < rhs._nanoseconds; }
  bool operator<=(const DateAndTime &rhs) const;
  bool operator>(const DateAndTime &rhs) const;
  bool operator>=(const DateAndTime &rhs) const;
  bool equals(const DateAndTime &rhs, const int64_t tol = 1) const;

  DateAndTime operator+(const int64_t nanosec) const;
  DateAndTime &operator+=(const int64_t nanosec);
  DateAndTime operator-(const int64_t nanosec) const;
  DateAndTime &operator-=(const int64_t nanosec);

  DateAndTime operator+(const uint64_t nanosec) const;

  DateAndTime operator+(const time_duration &td) const;
  DateAndTime &operator+=(const time_duration &td);
  DateAndTime operator-(const time_duration &td) const;
  DateAndTime &operator-=(const time_duration &td);

  DateAndTime operator+(const double sec) const;
  DateAndTime &operator+=(const double sec);
  DateAndTime operator-(const double sec) const;
  DateAndTime &operator-=(const double sec);

  time_duration operator-(const DateAndTime &rhs) const;

  //-------------- STATIC FUNCTIONS -----------------------
  static DateAndTime getCurrentTime();
  static DateAndTime maximum();
  static DateAndTime minimum();
  static double secondsFromDuration(const time_duration &duration);
  static time_duration durationFromSeconds(double duration);
  static int64_t nanosecondsFromDuration(const time_duration &td);
  static int64_t nanosecondsFromSeconds(double sec);
  static time_duration durationFromNanoseconds(int64_t dur);
  static const DateAndTime &defaultTime();
  static void createVector(const DateAndTime start, const std::vector<double> &seconds, std::vector<DateAndTime> &out);

  /// The difference in seconds between standard unix and gps epochs.
  static const uint32_t EPOCH_DIFF;

  /// The epoch for GPS times.
  static const boost::posix_time::ptime GPS_EPOCH;

  /// Const of one second time duration
  static const time_duration ONE_SECOND;

  static time_t utc_mktime(const struct tm *utctime);

private:
  /// A signed 64-bit int of the # of nanoseconds since Jan 1, 1990.
  int64_t _nanoseconds;

  /// Max allowed nanoseconds in the time; 2^62-1
  static constexpr int64_t MAX_NANOSECONDS = 4611686018427387903LL;

  /// Min allowed nanoseconds in the time; -2^62+1
  static constexpr int64_t MIN_NANOSECONDS = -4611686018427387903LL;
};

/** Default, empty constructor */
inline DateAndTime::DateAndTime() : _nanoseconds(0) {}

/** Construct a date from nanoseconds.
 * @param total_nanoseconds :: nanoseconds since Jan 1, 1990 (our epoch).
 */
inline DateAndTime::DateAndTime(const int64_t total_nanoseconds)
    : _nanoseconds{std::clamp(std::move(total_nanoseconds), MIN_NANOSECONDS, MAX_NANOSECONDS)} {
  // Make sure that you cannot construct a date that is beyond the limits...
}

/** + operator to add time.
 * @param nanosec :: number of nanoseconds to add
 * @return modified DateAndTime.
 */
inline DateAndTime DateAndTime::operator+(const int64_t nanosec) const { return DateAndTime(_nanoseconds + nanosec); }

inline DateAndTime DateAndTime::operator+(const uint64_t nanosec) const {
  return DateAndTime(_nanoseconds + static_cast<int64_t>(nanosec));
}

/** + operator to add time.
 * @param sec :: duration to add
 * @return modified DateAndTime.
 */
inline DateAndTime DateAndTime::operator+(const double sec) const {
  return this->operator+(nanosecondsFromSeconds(sec));
}

/** Nanoseconds from seconds, with limits
 * @param sec :: duration in seconds, as a double
 * @return int64 of the number of nanoseconds
 */
inline int64_t DateAndTime::nanosecondsFromSeconds(double sec) {
  const double nano = sec * 1e9;
  constexpr auto time_min = static_cast<double>(MIN_NANOSECONDS);
  constexpr auto time_max = static_cast<double>(MAX_NANOSECONDS);
  // Use these limits to avoid integer overflows
  if (nano > time_max)
    return MAX_NANOSECONDS;
  else if (nano < time_min)
    return MIN_NANOSECONDS;
  else
    return int64_t(nano);
}

} // namespace Core
} // namespace Types
} // namespace Mantid
