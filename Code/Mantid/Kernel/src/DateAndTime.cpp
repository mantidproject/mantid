#include "MantidKernel/DateAndTime.h"
#include <time.h>

namespace Mantid
{
namespace Kernel
{

namespace DateAndTime
{

//-----------------------------------------------------------------------------------------------
/** Portable implementation of gmtime_r (re-entrant gmtime)
 *
 */
std::tm * gmtime_r_portable( const std::time_t *clock, struct std::tm *result )
{
#ifdef _WIN32
  if (gmtime_s(result, clock))
  {
    return result;
  }
  else
  {
    return NULL;
  }
#else
  //Unix implementation
  return gmtime_r(clock, result);
#endif
}

/// utc_mktime() converts a struct tm that contains
/// broken down time in utc to a time_t.  This function uses
/// a brute-force method of conversion that does not require
/// the environment variable TZ to be changed at all, and is
/// therefore slightly more thread-safe in that regard.
///
/// The difference between mktime() and utc_mktime() is that
/// standard mktime() expects the struct tm to be in localtime,
/// according to the current TZ and system setting, while utc_mktime()
/// always assumes that the struct tm is in UTC, and converts it
/// to time_t regardless of what TZ is currently set.
///
/// The difference between utc_mktime() and TzWrapper::iso_mktime()
/// is that iso_mktime() will parse straight from an ISO string,
/// and if the ISO timestamp ends in a 'Z', it will behave like
/// utc_mktime() except it will alter the TZ environment variable
/// to do it.  If the ISO timestamp has no 'Z', then iso_mktime()
/// behaves like mktime().
///
///    Copyright (C) 2010, Chris Frey <cdfrey@foursquare.net>, To God be the glory
///    Released to the public domain.
time_t utc_mktime(struct tm *utctime)
{
  time_t result;
  struct tm tmp, check;

  // loop, converting "local time" to time_t and back to utc tm,
  // and adjusting until there are no differences... this
  // automatically takes care of DST issues.

  // do first conversion
  tmp = *utctime;
  tmp.tm_isdst = -1;
  result = mktime(&tmp);
  if( result == (time_t)-1 )
    return (time_t)-1;
  if( gmtime_r_portable(&result, &check) == NULL )
    return (time_t)-1;

  // loop until match
  while(  check.tm_year != utctime->tm_year ||
    check.tm_mon != utctime->tm_mon ||
    check.tm_mday != utctime->tm_mday ||
    check.tm_hour != utctime->tm_hour ||
    check.tm_min != utctime->tm_min )
  {
    tmp.tm_min  += utctime->tm_min - check.tm_min;
    tmp.tm_hour += utctime->tm_hour - check.tm_hour;
    tmp.tm_mday += utctime->tm_mday - check.tm_mday;
    tmp.tm_year += utctime->tm_year - check.tm_year;
    tmp.tm_isdst = -1;

    result = mktime(&tmp);
    if( result == (time_t)-1 )
      return (time_t)-1;
    gmtime_r_portable(&result, &check);
    if( gmtime_r_portable(&result, &check) == NULL )
      return (time_t)-1;
  }

  return result;
}


//-----------------------------------------------------------------------------------------------
/**
 * Return the number of seconds in a duration, as a double, including fractional seconds.
 */
double durationInSeconds(time_duration duration)
{
  return static_cast<double>(duration.total_seconds()) + static_cast<double>(duration.total_nanoseconds()) / 1e9;
}

//-----------------------------------------------------------------------------------------------
/// Create dateAndTime instance from a ISO 8601 yyyy-mm-ddThh:mm:ss input string
dateAndTime create_DateAndTime_FromISO8601_String(const std::string &str)
{
  //Make a copy
  std::string time = str;
  //Replace "T" with a space
  size_t n = time.find('T');
  if (n != std::string::npos)
    time[n] = ' ';
  //The boost conversion will handle it
  return boost::posix_time::time_from_string(time);
}

//-----------------------------------------------------------------------------------------------
/// Create a ISO 8601 yyyy-mm-ddThh:mm:ss string from a time
std::string create_ISO8601_String(const dateAndTime &time)
{
  char buffer [25];
  std::tm time_tm = boost::posix_time::to_tm(time); //turn into that struct
  strftime (buffer,25,"%Y-%m-%dT%H:%M:%S", &time_tm); //Make into a string
  return std::string(buffer);
}


//-----------------------------------------------------------------------------------------------
/** Convert a dateAndTime object to a std::tm time structure, using whatever time zone in the
 * dateAndtime (should be UTC) .
 */
std::tm to_tm(const dateAndTime &time)
{
  std::tm as_tm = boost::posix_time::to_tm(time);
  return as_tm;
}

//-----------------------------------------------------------------------------------------------
/** Returns the current dateAndTime, in UTC time, with microsecond precision
 *
 */
dateAndTime get_current_time()
{
return boost::posix_time::microsec_clock::universal_time();
}


//-----------------------------------------------------------------------------------------------
/** Returns a dateAndTime object made from a time_t input.
 * If the input time_t is in UTC, then the output will be as well.
 */
dateAndTime from_time_t(const std::time_t &time)
{
  return boost::posix_time::from_time_t( time );
}

//-----------------------------------------------------------------------------------------------
/** Convert a dateAndTime object (in UTC time) to std::time_t, in UTC time.
 */
std::time_t to_time_t(const dateAndTime &time)
{
  std::tm as_tm = boost::posix_time::to_tm(time);
  std::time_t as_time_t = utc_mktime( &as_tm );
  return as_time_t;
}


} //namespace  DateAndTime






TimeInterval::TimeInterval(const dateAndTime& from, const dateAndTime& to)
:m_begin(from)
{
    if (to > from) m_end = to;
    else
        m_end = from;
}

/**  Returns an intersection of this interval with \a ti
     @param ti Time interval 
     @return A valid time interval if this interval intersects with \a ti or 
             an empty interval otherwise.
 */
TimeInterval TimeInterval::intersection(const TimeInterval& ti)const
{
    if (!isValid() || !ti.isValid()) return TimeInterval();

    dateAndTime t1 = begin();
    if (ti.begin() > t1) t1 = ti.begin();

    dateAndTime t2 = end();
    if (ti.end() < t2) t2 = ti.end();

    return t1 < t2? TimeInterval(t1,t2) : TimeInterval();

}

/// String representation of the begin time
std::string TimeInterval::begin_str()const
{
  return boost::posix_time::to_simple_string(this->m_begin);

//    char buffer [25];
//    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&m_begin));
////    strftime (buffer,25,"%H:%M:%S",localtime(&m_begin));
//    return std::string(buffer);
}

/// String representation of the end time
std::string TimeInterval::end_str()const
{
  return boost::posix_time::to_simple_string(this->m_end);

//    char buffer [25];
//    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&m_end));
//    return std::string(buffer);
}


} // namespace Kernel
} // namespace Mantid

std::ostream& operator<<(std::ostream& s,const Mantid::Kernel::TimeInterval& t)
{
  s << t.begin() << " - " << t.end();
  return s;
//    char buffer [25];
//    Mantid::Kernel::dateAndTime d = t.begin();
//    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&d));
//    s<<buffer<<" - ";
//    d = t.end();
//    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&d));
//    s<<buffer;
//    return s;
}
