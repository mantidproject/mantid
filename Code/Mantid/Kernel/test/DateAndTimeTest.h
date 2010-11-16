/*
 * DateAndTimeTest.h
 *
 *  Created on: Aug 30, 2010
 *      Author: janik
 */

#ifndef DATEANDTIMETEST_H_
#define DATEANDTIMETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"
#include <sys/stat.h>
#include <time.h>
#include <sstream>

using namespace Mantid;
using namespace Mantid::Kernel;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;
using std::endl;

//==========================================================================================
class DateAndTimeTest: public CxxTest::TestSuite
{
public:

  void test_constructors_and_set()
  {
    //Expected will be Jan 2, 1990, at 00:01:02
    boost::posix_time::ptime expected = boost::posix_time::from_iso_string(std::string("19900102T000102.345"));
    DateAndTime d;
    //1day, 1 minute, 2 seconds, 0.345 seconds = 86462345000000 nanosec
    //Nanoseconds constructor
    d = DateAndTime(86462345000000);
    TS_ASSERT_EQUALS(d, expected);

    //Second, nanosec constructor
    d = DateAndTime(86462, 345000000);
    TS_ASSERT_EQUALS(d, expected);
    d = DateAndTime(86462.0, 345000000.0);
    TS_ASSERT_EQUALS(d, expected);
    //ptime
    d = DateAndTime(expected);
    TS_ASSERT_EQUALS(d, expected);

    //string
    d = DateAndTime("1990-01-02T00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);
    d.set_from_ISO8601_string("1990-01-02T00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);

    //string with a space
    d = DateAndTime("1990-01-02 00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);
    d.set_from_ISO8601_string("1990-01-02 00:01:02.345");
    TS_ASSERT_EQUALS(d, expected);
  }


  void test_limits_on_construction()
  {
    DateAndTime a,b,c;
    a = DateAndTime(6917529027641081856);
    TS_ASSERT_EQUALS( a, DateAndTime::maximum());
    a = DateAndTime(-6917529027641081856);
    TS_ASSERT_EQUALS( a, DateAndTime::minimum());
    a = DateAndTime(1e20, 0.2);
    TS_ASSERT_EQUALS( a, DateAndTime::maximum());
    a = DateAndTime(-1e20, 0.2);
    TS_ASSERT_EQUALS( a, DateAndTime::minimum());

    a = DateAndTime("2490-01-02 00:01:02.345");
    TS_ASSERT_EQUALS( a, DateAndTime::maximum());
    a = DateAndTime("1600-01-02 00:01:02.345");
    TS_ASSERT_EQUALS( a, DateAndTime::minimum());
  }

  void test_year_month_etc()
  {
    DateAndTime a;
    a = DateAndTime("1990-01-02 03:04:05.678");
    TS_ASSERT_EQUALS( a.year(), 1990);
    TS_ASSERT_EQUALS( a.month(), 1);
    TS_ASSERT_EQUALS( a.day(), 2);
    TS_ASSERT_EQUALS( a.hour(), 3);
    TS_ASSERT_EQUALS( a.minute(), 4);
    TS_ASSERT_EQUALS( a.second(), 5);
    TS_ASSERT_EQUALS( a.nanoseconds(), 678000000);
  }

  void test_to_string()
  {
    DateAndTime a;
    a = DateAndTime("1990-01-02 03:04:05.678");
    std::string s = a.to_simple_string();
    TS_ASSERT_EQUALS( s.substr(0,20), "1990-Jan-02 03:04:05");
    TS_ASSERT_EQUALS( a.to_string(), "1990-Jan-02 03:04:05");
    TS_ASSERT_EQUALS( a.to_string("%Y-%m-%d"), "1990-01-02");
  }

  void test_stream_operator()
  {
    DateAndTime a;
    std::ostringstream message;
    a = DateAndTime("1990-01-02 03:04:05.678");
    message << a;
    TS_ASSERT_EQUALS( message.str(), a.to_simple_string() );
  }


  void test_subtraction_of_times()
  {
    DateAndTime a,b,c;
    boost::posix_time::ptime p;
    time_duration td;

    a = DateAndTime("1990-01-02 00:01:02.345");
    b = DateAndTime("1990-01-02 00:00:02.000");
    td = a-b;
    TS_ASSERT_EQUALS( td, DateAndTime::duration_from_nanoseconds(60345000000) );

    a = DateAndTime("1990-01-02 00:01:02.345");
    p = boost::posix_time::from_iso_string("19900102T000002.000");
    //boost ptime gets converted to ptime implicitely
    td = a-p;
    TS_ASSERT_EQUALS( td, DateAndTime::duration_from_nanoseconds(60345000000) );
  }


  void test_addition_and_subtraction_operators_time_duration()
  {
    DateAndTime a,b,c;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = a + DateAndTime::duration_from_nanoseconds(60345000000);
    TS_ASSERT_EQUALS( c, b);
    a += DateAndTime::duration_from_nanoseconds(60345000000);
    TS_ASSERT_EQUALS( a, b);

    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = b - DateAndTime::duration_from_nanoseconds(60345000000);
    TS_ASSERT_EQUALS( c, a);
    b -= DateAndTime::duration_from_nanoseconds(60345000000);
    TS_ASSERT_EQUALS( b, a);
  }



  void test_addition_and_subtraction_operators_double()
  {
    DateAndTime a,b,c;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = a + 60.345;
    TS_ASSERT_EQUALS( c, b);
    a += 60.345;
    TS_ASSERT_EQUALS( a, b);

    a = DateAndTime("1990-01-02 00:00:02.000");
    b = DateAndTime("1990-01-02 00:01:02.345");
    c = b - 60.345;
    TS_ASSERT_EQUALS( c, a);
    b -= 60.345;
    TS_ASSERT_EQUALS( b, a);
  }

  void test_limits_on_addition_and_subtraction()
  {
    DateAndTime a,b,c;
    a = DateAndTime("1990-01-02 00:00:02.000");
    b = a + 1e20;
    TS_ASSERT_EQUALS( b, DateAndTime::maximum());
    b = a - 1e20;
    TS_ASSERT_LESS_THAN( b.year(), 1900);

    a = DateAndTime("1989-01-02 00:00:02.000");
    b = a - 1e20;
    TS_ASSERT_EQUALS( b, DateAndTime::minimum());
    b = a + 1e20;
    TS_ASSERT_LESS_THAN( 2000, b.year());
  }


  void test_dataSizes()
  {
    //Must occupy 8 bytes!
    TS_ASSERT_EQUALS( sizeof( DateAndTime ), 8 );
  }

  void test_time_t_support()
  {
    DateAndTime t;
    t.set_from_time_t( 5 );
    TS_ASSERT_EQUALS( 5, t.to_time_t() );
  }

  void testCurrentTime()
  {
    //Use the c-method to get current (local) time
    std::time_t current_t = DateAndTime::get_current_time().to_time_t() ;
    std::tm * current = gmtime( &current_t );
    //std::cout << "UTC time is " << current->tm_hour << "h" << current->tm_min << "\n";
    //Compare
    TS_ASSERT( current->tm_year >= 110 ); //Wrote this in 2010, so the year must be > 110
  }



  void test_timezones()
  {
    int hour = 12;

    std::time_t rawtime;
    std::time(&rawtime); //current time will be overwritten

    std::tm * timeinfo = new std::tm;
    timeinfo->tm_isdst = -1;
    timeinfo->tm_year = 108;
    timeinfo->tm_mon = 1;
    timeinfo->tm_mday = 29;
    timeinfo->tm_hour = hour;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    //Convert to time_t but assuming the tm is specified in UTC time.
    std::time_t utc_time_t =  Mantid::Kernel::DateAndTimeHelpers::utc_mktime ( timeinfo );
    //This will be the local time
    std::time_t local_time_t =  std::mktime( timeinfo );

    //our format, as utc
    DateAndTime utc_time;
    utc_time.set_from_time_t(utc_time_t);

    //Timezone offset in hours (sorry, newfoundland and labrador - half time zones are crazy! )
    int tz_offset = static_cast<int>( difftime(utc_time_t, local_time_t) / 3600 );

    //Get tm in UTC
    std::tm utc_tm = utc_time.to_tm();
    TS_ASSERT_EQUALS( utc_tm.tm_hour, hour);

    //Get tm in localtime
    std::tm local_tm = utc_time.to_localtime_tm();
    TS_ASSERT_EQUALS( local_tm.tm_hour, hour + tz_offset);

    //Now the time_t conversion, UTC time
    TS_ASSERT_EQUALS( utc_time.to_time_t(), utc_time_t);

    //Now the time_t conversion, local time
    TS_ASSERT_EQUALS( utc_time.to_localtime_t(), local_time_t);

    //Now the string
    TS_ASSERT_EQUALS( utc_time.to_simple_string(), "2008-Feb-29 12:00:00");

  }


  void test_ISO8601_string_with_timezones()
  {
    //Time without timezone : UTC assumed
    DateAndTime time_no_tz = DateAndTime("2010-03-24T14:12:51.562");
    DateAndTime time_no_fraction = DateAndTime("2010-03-24T14:12:51");

    //The conversion should handle the fraction
    TS_ASSERT_DELTA(  Mantid::Kernel::DateAndTime::seconds_from_duration( time_no_tz-time_no_fraction ), 0.562, 0.0005);

    //ZULU specified
    DateAndTime time_z = DateAndTime("2010-03-24T14:12:51.562Z");
    //Positive time offset (also a fraction like Newfoundland (crazy newfies ;) )
    DateAndTime time_positive_tz = DateAndTime("2010-03-24T19:42:51.562+05:30");
    DateAndTime time_positive_tz2 = DateAndTime("2010-03-24T16:12:51.562+02");
    //Negative time offset
    DateAndTime time_negative_tz = DateAndTime("2010-03-24T10:12:51.562-04:00");
    DateAndTime time_negative_tz2 = DateAndTime("2010-03-24T06:12:51.562-08");


    //Now check the time zone difference
    TS_ASSERT_DELTA(  Mantid::Kernel::DateAndTime::seconds_from_duration( time_no_tz-time_z ),  0.0, 1e-4);
    TS_ASSERT_DELTA(  Mantid::Kernel::DateAndTime::seconds_from_duration( time_no_tz-time_positive_tz ),  0.0, 1e-4);
    TS_ASSERT_DELTA(  Mantid::Kernel::DateAndTime::seconds_from_duration( time_no_tz-time_negative_tz ),  0.0, 1e-4);
    TS_ASSERT_DELTA(  Mantid::Kernel::DateAndTime::seconds_from_duration( time_no_tz-time_positive_tz2 ),  0.0, 1e-4);
    TS_ASSERT_DELTA(  Mantid::Kernel::DateAndTime::seconds_from_duration( time_no_tz-time_negative_tz2 ),  0.0, 1e-4);
  }


  void testDurations()
  {
    time_duration onesec = time_duration(0,0,1,0);
    TS_ASSERT_EQUALS( DateAndTime::seconds_from_duration(onesec), 1.0 );

    onesec = DateAndTime::duration_from_seconds(1.0);
    TS_ASSERT_EQUALS( DateAndTime::seconds_from_duration(onesec), 1.0 );

    time_duration td = DateAndTime::duration_from_seconds(1e-6);
    TS_ASSERT_DELTA( DateAndTime::seconds_from_duration(td), 1e-6, 1e-9 );

    //Now difference between dates
    DateAndTime dt = DateAndTime(0);
    DateAndTime dt2 = dt + td;
    TS_ASSERT_DELTA( DateAndTime::seconds_from_duration(dt2-dt), 1e-6, 1e-9 );

    td = DateAndTime::duration_from_seconds(12.345);
    TS_ASSERT_DELTA( DateAndTime::seconds_from_duration(td), 12.345, 1e-9 );


    dt2 = dt + DateAndTime::duration_from_seconds(123.5e-3);
    TS_ASSERT_DELTA( DateAndTime::seconds_from_duration(dt2-dt), 123.5e-3, 1e-9 );

    dt2 = dt + DateAndTime::duration_from_seconds(15.2345);
    TS_ASSERT_DELTA( DateAndTime::seconds_from_duration(dt2-dt), 15.2345, 1e-9 );

    dt2 = dt + DateAndTime::duration_from_seconds(152.345);
    TS_ASSERT_DELTA( DateAndTime::seconds_from_duration(dt2-dt), 152.345, 1e-9 );
  }


  /* Ensure that exceptions thrown by boost date_time conversions are caught where they
     may cause problems. */
  void testNotADateTime()
  {
    boost::posix_time::ptime time(boost::posix_time::not_a_date_time);
    DateAndTime dt(time);
    TS_ASSERT_THROWS(std::tm tm = boost::posix_time::to_tm(time), std::out_of_range);
    TS_ASSERT_THROWS_NOTHING(std::tm tm2 = dt.to_tm());
  }

  void test_duration_limits()
  {
    DateAndTime a,b,c,d;
    time_duration td;
    a = DateAndTime("2010-03-24T14:12:51.562");
    // Only about 290 years time difference are supported (2^63 nanoseconds)!
    b = DateAndTime("2300-03-24T14:12:51.562");
    td = b-a;
    c = a + td;
    TS_ASSERT_EQUALS( c, b);
  }


  void test_duration_from_seconds_Extremes()
  {
    time_duration onesec = time_duration(0,0,1,0);
    time_duration extreme;
    extreme = DateAndTime::duration_from_seconds(1e20);
    TS_ASSERT_LESS_THAN(onesec, extreme);

    extreme = DateAndTime::duration_from_seconds(-1e20);
    TS_ASSERT_LESS_THAN(extreme, onesec);
  }

};



#endif /* DATEANDTIMETEST_H_ */



