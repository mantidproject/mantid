/*
 * DateAndTimeTest.h
 *
 *  Created on: Aug 30, 2010
 *      Author: janik
 */

#ifndef DATEANDTIMETEST_H_
#define DATEANDTIMETEST_H_

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"
#include <ctime>
#include <cxxtest/TestSuite.h>
#include <sstream>
#include <sys/stat.h>

using namespace Mantid;
using namespace Mantid::Kernel;

using std::runtime_error;
using std::size_t;
using std::vector;

//==========================================================================================
class DateAndTimeTest : public CxxTest::TestSuite {
public:
  void test_constructors_and_set() {
    // Expected will be Jan 2, 1990, at 00:01:02
    boost::posix_time::ptime expected =
        boost::posix_time::from_iso_string(std::string("19900102T000102.345"));
    DateAndTime d;
    // 1day, 1 minute, 2 seconds, 0.345 seconds = 86462345000000 nanosec
    // Nanoseconds constructor
    d = DateAndTime(int64_t(86462345000000LL));
    TS_ASSERT_EQUALS(d, expected);

    // Second, nanosec constructor
    d = DateAndTime(86462, 345000000);
    TS_ASSERT_EQUALS(d, expected);
    d = DateAndTime(86462.0, 345000000.0);
    TS_ASSERT_EQUALS(d, expected);
    // ptime
    d = DateAndTime(expected);
    TS_ASSERT_EQUALS(d, expected);

    // string
    d = DateAndTime("1990-01-02T00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);
    d.setFromISO8601("1990-01-02T00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);

    // string with a space
    d = DateAndTime("1990-01-02 00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);
    d.setFromISO8601("1990-01-02 00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);
  }

  void test_constructor_fails_invalid_string() {
    TS_ASSERT_THROWS(DateAndTime("invalid time string"), std::invalid_argument);
    TS_ASSERT_THROWS(DateAndTime("1909-01-31  22:59:59"),
                     std::invalid_argument);
    TS_ASSERT_THROWS(DateAndTime("2017-09-27T 07:03:49+00:00"),
                     std::invalid_argument);
  }

  void test_limits_on_construction() {
    // direct nanoseconds constructor
    DateAndTime a, b, c;
    a = DateAndTime(int64_t(6917529027641081856LL));
    TS_ASSERT_EQUALS(a, DateAndTime::maximum());
    a = DateAndTime(int64_t(-6917529027641081856LL));

    // Double constructor
    TS_ASSERT_EQUALS(a, DateAndTime::minimum());
    a = DateAndTime(1e20, 0.2);
    TS_ASSERT_EQUALS(a, DateAndTime::maximum());
    a = DateAndTime(-1e20, 0.2);
    TS_ASSERT_EQUALS(a, DateAndTime::minimum());

    // long int constructor
    int64_t li = 1000000000000000000LL;
    int64_t li2 = 2000000LL;
    a = DateAndTime(li, li2);
    TS_ASSERT_EQUALS(a, DateAndTime::maximum());
    a = DateAndTime(-li, li2);
    TS_ASSERT_EQUALS(a, DateAndTime::minimum());

    // String constructors
    a = DateAndTime("2490-01-02 00:01:02.345");
    TS_ASSERT_EQUALS(a, DateAndTime::maximum());
    a = DateAndTime("1600-01-02 00:01:02.345");
    TS_ASSERT_EQUALS(a, DateAndTime::minimum());

    // ptime constructor
    boost::posix_time::ptime p;
    p = boost::posix_time::from_iso_string("24000102T000102");
    a = DateAndTime(p);
    TS_ASSERT_EQUALS(a, DateAndTime::maximum());
    p = boost::posix_time::from_iso_string("16000102T000102");
    a = DateAndTime(p);
    TS_ASSERT_EQUALS(a, DateAndTime::minimum());

    // time_t and int constructors can't overflow on 32-bits at least
  }

  void test_year_month_etc() {
    DateAndTime a;
    a = DateAndTime("1990-01-02 03:04:05.678");
    TS_ASSERT_EQUALS(a.year(), 1990);
    TS_ASSERT_EQUALS(a.month(), 1);
    TS_ASSERT_EQUALS(a.day(), 2);
    TS_ASSERT_EQUALS(a.hour(), 3);
    TS_ASSERT_EQUALS(a.minute(), 4);
    TS_ASSERT_EQUALS(a.second(), 5);
    TS_ASSERT_EQUALS(a.nanoseconds(), 678000000);
  }

  void test_comparison_operators() {
    DateAndTime a, b, c, d;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = DateAndTime("1990-01-02 00:01:02.345");
    d = DateAndTime("1990-01-02 00:00:02.000000001");

    TS_ASSERT(a < b);
    TS_ASSERT(b > a);
    TS_ASSERT(a <= b);
    TS_ASSERT(b >= a);
    TS_ASSERT(a == a);
    TS_ASSERT(b == b);
    TS_ASSERT(b == c);
    TS_ASSERT(a != b);
    // intentionally different to confirm the tolerance check works
    TS_ASSERT(a != d);
    TS_ASSERT(a.equals(d));

    boost::posix_time::ptime p;
    p = boost::posix_time::from_iso_string("19900102T000002.000");
    TS_ASSERT(a == p);
    TS_ASSERT(b != p);
  }

  void test_toFormattedString() {
    DateAndTime a;
    a = DateAndTime("1990-01-02 03:04:05.678");
    std::string s = a.toSimpleString();
    TS_ASSERT_EQUALS(s.substr(0, 20), "1990-Jan-02 03:04:05");
    TS_ASSERT_EQUALS(a.toFormattedString(), "1990-Jan-02 03:04:05");
    TS_ASSERT_EQUALS(a.toFormattedString("%Y-%m-%d"), "1990-01-02");
    TS_ASSERT_EQUALS(a.toISO8601String(), "1990-01-02T03:04:05.678000000");
  }

  void test_to_int64() {
    DateAndTime a;
    a = DateAndTime("1990-01-02 00:01:02.345");
    int64_t nanosec = a.totalNanoseconds();
    // 1day, 1 minute, 2 seconds, 0.345 seconds = 86462345000000 nanosec
    TS_ASSERT_EQUALS(nanosec, int64_t(86462345000000LL));
  }

  void test_stream_operator() {
    DateAndTime a;
    std::ostringstream message;
    a = DateAndTime("1990-01-02 03:04:05.678");
    message << a;
    TS_ASSERT_EQUALS(message.str(), a.toSimpleString());
    std::ostringstream message2;
    message2 << a << "\n";
    TS_ASSERT_EQUALS(message2.str(), a.toSimpleString() + "\n");
  }

  void test_subtraction_of_times() {
    DateAndTime a, b, c;
    boost::posix_time::ptime p;
    time_duration td;

    a = DateAndTime("1990-01-02 00:01:02.345");
    b = DateAndTime("1990-01-02 00:00:02.000");
    td = a - b;
    TS_ASSERT_EQUALS(
        td, DateAndTime::durationFromNanoseconds(int64_t(60345000000LL)));

    a = DateAndTime("1990-01-02 00:01:02.345");
    p = boost::posix_time::from_iso_string("19900102T000002.000");
    // boost ptime gets converted to ptime implicitely
    td = a - p;
    TS_ASSERT_EQUALS(
        td, DateAndTime::durationFromNanoseconds(int64_t(60345000000LL)));
  }

  void test_subtraction_of_times_limits() {
    DateAndTime a, b, c;
    boost::posix_time::ptime p;
    time_duration td;

    a = DateAndTime("2200-01-02 00:01:02.345");
    b = DateAndTime("1800-01-02 00:01:02.345");
    td = a - b;
    // The difference won't be correct, but it is positive and ~2**62
    // nanoseconds
    TS_ASSERT_LESS_THAN(4.6e9, DateAndTime::secondsFromDuration(td));

    td = b - a;
    // The difference won't be correct, but it is negative
    TS_ASSERT_LESS_THAN(DateAndTime::secondsFromDuration(td), -4.6e9);
  }

  void test_addition_and_subtraction_operators_nanoseconds_as_int() {
    DateAndTime a, b, c;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = a + int64_t(60345000000LL);
    TS_ASSERT_EQUALS(c, b);
    a += int64_t(60345000000LL);
    TS_ASSERT_EQUALS(a, b);

    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = b - int64_t(60345000000LL);
    TS_ASSERT_EQUALS(c, a);
    b -= int64_t(60345000000LL);
    TS_ASSERT_EQUALS(b, a);
  }

  void test_addition_and_subtraction_operators_time_duration() {
    DateAndTime a, b, c;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = a + DateAndTime::durationFromNanoseconds(int64_t(60345000000LL));
    TS_ASSERT_EQUALS(c, b);
    a += DateAndTime::durationFromNanoseconds(int64_t(60345000000LL));
    TS_ASSERT_EQUALS(a, b);

    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = b - DateAndTime::durationFromNanoseconds(int64_t(60345000000LL));
    TS_ASSERT_EQUALS(c, a);
    b -= DateAndTime::durationFromNanoseconds(int64_t(60345000000LL));
    TS_ASSERT_EQUALS(b, a);
  }

  void test_addition_and_subtraction_operators_double() {
    DateAndTime a, b, c;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = a + 60.345;
    TS_ASSERT_EQUALS(c, b);
    a += 60.345;
    TS_ASSERT_EQUALS(a, b);

    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = b - 60.345;
    TS_ASSERT_EQUALS(c, a);
    b -= 60.345;
    TS_ASSERT_EQUALS(b, a);
  }

  void test_limits_on_addition_and_subtraction() {
    DateAndTime a, b, c;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = a + 1e20;
    TS_ASSERT_EQUALS(b, DateAndTime::maximum());
    b = a - 1e20;
    TS_ASSERT_LESS_THAN(b.year(), 1900);

    a = DateAndTime("1989-01-02 00:00:02.000");
    b = a - 1e20;
    TS_ASSERT_EQUALS(b, DateAndTime::minimum());
    b = a + 1e20;
    TS_ASSERT_LESS_THAN(2000, b.year());
  }

  void test_dataSizes() {
    // Must occupy 8 bytes!
    TS_ASSERT_EQUALS(sizeof(DateAndTime), 8);
  }

  void test_time_t_support() {
    DateAndTime t;
    std::time_t current = time(nullptr);
    t.set_from_time_t(current);
    //    if (cur.day() < 28) // Annoying bug at the end of a month
    { TS_ASSERT_EQUALS(current, t.to_time_t()); }
  }

  void testCurrentTime() {
    // Use the c-method to get current (local) time
    std::time_t current_t = DateAndTime::getCurrentTime().to_time_t();
    std::tm *current = gmtime(&current_t);
    // std::cout << "UTC time is " << current->tm_hour << "h" << current->tm_min
    // << "\n";
    // Compare
    TS_ASSERT(current->tm_year >=
              110); // Wrote this in 2010, so the year must be > 110
  }

  void test_timezones() {
    int hour = 12;

    std::time_t rawtime;
    std::time(&rawtime); // current time will be overwritten

    std::tm *timeinfo = new std::tm;
    timeinfo->tm_isdst = -1;
    timeinfo->tm_year = 108;
    timeinfo->tm_mon = 1;
    timeinfo->tm_mday = 29;
    timeinfo->tm_hour = hour;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    // Convert to time_t but assuming the tm is specified in UTC time.
    std::time_t utc_time_t = Mantid::Kernel::DateAndTime::utc_mktime(timeinfo);
    // This will be the local time
    std::time_t local_time_t = std::mktime(timeinfo);

    // our format, as utc
    DateAndTime utc_time;
    utc_time.set_from_time_t(utc_time_t);

    // Timezone offset in hours (sorry, newfoundland and labrador - half time
    // zones are crazy! )
    int tz_offset = static_cast<int>(difftime(utc_time_t, local_time_t) / 3600);

    // Get tm in UTC
    std::tm utc_tm = utc_time.to_tm();
    TS_ASSERT_EQUALS(utc_tm.tm_hour, hour);

    // Get tm in localtime
    std::tm local_tm = utc_time.to_localtime_tm();
    TS_ASSERT_EQUALS(local_tm.tm_hour, hour + tz_offset);

    // Now the time_t conversion, UTC time
    TS_ASSERT_EQUALS(utc_time.to_time_t(), utc_time_t);

    // Now the time_t conversion, local time
    TS_ASSERT_EQUALS(utc_time.to_localtime_t(), local_time_t);

    // Now the string
    TS_ASSERT_EQUALS(utc_time.toSimpleString(), "2008-Feb-29 12:00:00");

    delete timeinfo;
  }

  void test_ISO8601_string_with_timezones() {
    // Time without timezone : UTC assumed
    DateAndTime time_no_tz = DateAndTime("2010-03-24T14:12:51.562");
    DateAndTime time_no_fraction = DateAndTime("2010-03-24T14:12:51");

    // The conversion should handle the fraction
    TS_ASSERT_DELTA(Mantid::Kernel::DateAndTime::secondsFromDuration(
                        time_no_tz - time_no_fraction),
                    0.562, 0.0005);

    // ZULU specified
    DateAndTime time_z = DateAndTime("2010-03-24T14:12:51.562Z");
    // Positive time offset (also a fraction like Newfoundland (crazy newfies ;)
    // )
    DateAndTime time_positive_tz = DateAndTime("2010-03-24T19:42:51.562+05:30");
    DateAndTime time_positive_tz2 = DateAndTime("2010-03-24T16:12:51.562+02");
    // Negative time offset
    DateAndTime time_negative_tz = DateAndTime("2010-03-24T10:12:51.562-04:00");
    DateAndTime time_negative_tz2 = DateAndTime("2010-03-24T06:12:51.562-08");

    // Now check the time zone difference
    TS_ASSERT_DELTA(
        Mantid::Kernel::DateAndTime::secondsFromDuration(time_no_tz - time_z),
        0.0, 1e-4);
    TS_ASSERT_DELTA(Mantid::Kernel::DateAndTime::secondsFromDuration(
                        time_no_tz - time_positive_tz),
                    0.0, 1e-4);
    TS_ASSERT_DELTA(Mantid::Kernel::DateAndTime::secondsFromDuration(
                        time_no_tz - time_negative_tz),
                    0.0, 1e-4);
    TS_ASSERT_DELTA(Mantid::Kernel::DateAndTime::secondsFromDuration(
                        time_no_tz - time_positive_tz2),
                    0.0, 1e-4);
    TS_ASSERT_DELTA(Mantid::Kernel::DateAndTime::secondsFromDuration(
                        time_no_tz - time_negative_tz2),
                    0.0, 1e-4);
  }

  void testDurations() {
    time_duration onesec = time_duration(0, 0, 1, 0);
    TS_ASSERT_EQUALS(DateAndTime::secondsFromDuration(onesec), 1.0);

    onesec = DateAndTime::durationFromSeconds(1.0);
    TS_ASSERT_EQUALS(DateAndTime::secondsFromDuration(onesec), 1.0);

    time_duration td = DateAndTime::durationFromSeconds(1e-6);
    TS_ASSERT_DELTA(DateAndTime::secondsFromDuration(td), 1e-6, 1e-9);

    // Now difference between dates
    DateAndTime dt = DateAndTime(0);
    DateAndTime dt2 = dt + td;
    TS_ASSERT_DELTA(DateAndTime::secondsFromDuration(dt2 - dt), 1e-6, 1e-9);

    td = DateAndTime::durationFromSeconds(12.345);
    TS_ASSERT_DELTA(DateAndTime::secondsFromDuration(td), 12.345, 1e-9);

    dt2 = dt + DateAndTime::durationFromSeconds(123.5e-3);
    TS_ASSERT_DELTA(DateAndTime::secondsFromDuration(dt2 - dt), 123.5e-3, 1e-9);

    dt2 = dt + DateAndTime::durationFromSeconds(15.2345);
    TS_ASSERT_DELTA(DateAndTime::secondsFromDuration(dt2 - dt), 15.2345, 1e-9);

    dt2 = dt + DateAndTime::durationFromSeconds(152.345);
    TS_ASSERT_DELTA(DateAndTime::secondsFromDuration(dt2 - dt), 152.345, 1e-9);
  }

  /* Ensure that exceptions thrown by boost date_time conversions are caught
     where they
     may cause problems. */
  void testNotADateTime() {
    boost::posix_time::ptime time(boost::posix_time::not_a_date_time);
    DateAndTime dt(time);
    TS_ASSERT_THROWS(boost::posix_time::to_tm(time), std::out_of_range);
    TS_ASSERT_THROWS_NOTHING(dt.to_tm());
  }

  void test_duration_limits() {
    DateAndTime a, b, c, d;
    time_duration td;
    a = DateAndTime("2010-03-24T14:12:51.562");
    // Only about 290 years time difference are supported (2^63 nanoseconds)!
    b = DateAndTime("2300-03-24T14:12:51.562");
    td = b - a;
    c = a + td;
    TS_ASSERT_EQUALS(c, b);
  }

  void test_duration_from_seconds_Extremes() {
    time_duration onesec = time_duration(0, 0, 1, 0);
    time_duration extreme;
    extreme = DateAndTime::durationFromSeconds(1e20);
    // Output value is positive
    TS_ASSERT_LESS_THAN(onesec, extreme);

    extreme = DateAndTime::durationFromSeconds(-1e20);
    // Output value is negative
    TS_ASSERT_LESS_THAN(extreme, onesec);
  }

  void test_Vector() {
    DateAndTime a = DateAndTime("1990-01-02 03:04:05.000");
    std::vector<double> secs{1.0, 2.0, 0.5, -3.0};

    std::vector<DateAndTime> times;
    DateAndTime::createVector(a, secs, times);
    TS_ASSERT_EQUALS(times.size(), secs.size());
    TS_ASSERT_EQUALS(times[0], DateAndTime("1990-01-02 03:04:06.000"));
    TS_ASSERT_EQUALS(times[1], DateAndTime("1990-01-02 03:04:07.000"));
    TS_ASSERT_EQUALS(times[2], DateAndTime("1990-01-02 03:04:05.500"));
    TS_ASSERT_EQUALS(times[3], DateAndTime("1990-01-02 03:04:02.000"));
  }
};

#endif /* DATEANDTIMETEST_H_ */
