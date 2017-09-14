/*
 * DateAndTimeTest.h
 *
 *  Created on: Aug 30, 2010
 *      Author: janik
 */

#ifndef DATEANDTIMETEST_H_
#define DATEANDTIMETEST_H_

#include "MantidTypes/DateAndTime.h"
#include <cxxtest/TestSuite.h>
#include <sstream>
#include <sys/stat.h>
#include <time.h>

using namespace Mantid;
using namespace Mantid::Types;

using std::cout;
using std::runtime_error;
using std::size_t;
using std::vector;

//==========================================================================================
class DateAndTimeTest : public CxxTest::TestSuite {
public:
  void test_constructors() {
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

  void test_dataSizes() {
    // Must occupy 8 bytes!
    TS_ASSERT_EQUALS(sizeof(DateAndTime), 8);
  }

  void test_time_t_support() {
    DateAndTime t;
    std::time_t current = time(NULL);
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
    std::time_t utc_time_t =
        Mantid::Types::DateAndTimeHelpers::utc_mktime(timeinfo);
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
};

#endif /* DATEANDTIMETEST_H_ */
