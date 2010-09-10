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

using namespace Mantid;
using namespace Mantid::Kernel;
using Mantid::Kernel::DateAndTime::get_time_from_pulse_time;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;
using std::endl;

//==========================================================================================
class DateAndTimeTest: public CxxTest::TestSuite
{
public:

  void testCurrentTime()
  {
    //Use the c-method to get current (local) time
    std::time_t current_t = DateAndTime::to_time_t( DateAndTime::get_current_time() );
    std::tm * current = gmtime( &current_t );
//    std::cout << "UTC time is " << current->tm_hour << "h" << current->tm_min << "\n";
    //Compare
    TS_ASSERT( current->tm_year >= 110 ); //Wrote this in 2010, so the
  }

  void test_conversions()
  {
    Mantid::Kernel::dateAndTime execTime = DateAndTime::get_current_time();
    std::time_t execTime_t = DateAndTime::to_time_t(execTime);
    TS_ASSERT_EQUALS( execTime_t, DateAndTime::to_time_t(execTime) ); //this one is trivial

    //Reverse the conversion
    TS_ASSERT_EQUALS( DateAndTime::to_time_t(execTime),  DateAndTime::to_time_t( DateAndTime::from_time_t(execTime_t)) );
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
    std::time_t utc_time_t =  Mantid::Kernel::DateAndTime::utc_mktime ( timeinfo );
    //This will be the local time
    std::time_t local_time_t =  std::mktime( timeinfo );
    //our format, as utc
    dateAndTime utc_time = Mantid::Kernel::DateAndTime::from_time_t(utc_time_t);

    //Timezone offset in hours (sorry, newfoundland and labrador - half time zones are crazy! )
    int tz_offset = difftime(utc_time_t, local_time_t) / 3600;

    //Get tm in UTC
    std::tm utc_tm = Mantid::Kernel::DateAndTime::to_tm(utc_time);
    TS_ASSERT_EQUALS( utc_tm.tm_hour, hour);

    //Get tm in localtime
    std::tm local_tm = Mantid::Kernel::DateAndTime::to_localtime_tm(utc_time);
    TS_ASSERT_EQUALS( local_tm.tm_hour, hour + tz_offset);

    //Now the time_t conversion, UTC time
    TS_ASSERT_EQUALS( Mantid::Kernel::DateAndTime::to_time_t(utc_time), utc_time_t);

    //Now the time_t conversion, local time
    TS_ASSERT_EQUALS( Mantid::Kernel::DateAndTime::to_localtime_t(utc_time), local_time_t);

    //Now the string
    TS_ASSERT_EQUALS( Mantid::Kernel::DateAndTime::to_simple_string(utc_time), "2008-Feb-29 12:00:00");

  }


  void test_get_time_from_pulse_time()
  {
    dateAndTime dt = DateAndTime::GPS_EPOCH;
    PulseTimeType pt = 0;
    TS_ASSERT_EQUALS( dt, get_time_from_pulse_time(pt));

    //Add one second
    dt += time_duration(0,0,1,0);
    pt = 1e9;
    TS_ASSERT_EQUALS( dt, get_time_from_pulse_time(pt));

    //Half a second
    dt += boost::posix_time::milliseconds(500);
    pt += 5e8;
    TS_ASSERT_EQUALS( dt, get_time_from_pulse_time(pt));

    //All installs should support at least 1 microsecond resolution.
    dt += boost::posix_time::microseconds(1);
    pt += 1e3;
    TS_ASSERT_EQUALS( dt, get_time_from_pulse_time(pt));

//    std::cout << DateAndTime::to_simple_string(get_time_from_pulse_time(pt)) << "\n";
  }


};



#endif /* DATEANDTIMETEST_H_ */



