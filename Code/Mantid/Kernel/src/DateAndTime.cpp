#include "MantidKernel/DateAndTime.h"
#include <time.h>

namespace Mantid
{
namespace Kernel
{

namespace DateAndTime
{

//-----------------------------------------------------------------------------------------------
/** Convert time_t to tm as UTC time.
 * Portable implementation of gmtime_r (re-entrant gmtime) that works on Windows and Linux
 *
 * @param clock pointer to time_t to convert
 * @param result pointer to a struct tm (timeinfo structure) that will be filled.
 * @return result if successful, or NULL if there was an error.
 */
std::tm * gmtime_r_portable( const std::time_t *clock, struct std::tm *result )
{
#ifdef _WIN32
  //Windows implementation
  if (!gmtime_s(result, clock))
  { //Returns zero if successful
    return result;
  }
  else
  { //Returned some non-zero error code
    return NULL;
  }
#else
  //Unix implementation
  return gmtime_r(clock, result);
#endif
}

//-----------------------------------------------------------------------------------------------
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
  int counter = 0;
  while( counter < 15 &&
  ( check.tm_year != utctime->tm_year ||
    check.tm_mon != utctime->tm_mon ||
    check.tm_mday != utctime->tm_mday ||
    check.tm_hour != utctime->tm_hour ||
    check.tm_min != utctime->tm_min ) )
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
    //Seems like there can be endless loops at the end of a month? E.g. sep 30, 2010 at 4:40 pm. This is to avoid it.
    counter++;
  }
  return result;
}


//-----------------------------------------------------------------------------------------------
/**
 * Return the number of seconds in a duration, as a double, including fractional seconds.
 */
double durationInSeconds(time_duration duration)
{
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  // Nanosecond resolution
  return static_cast<double>(duration.total_nanoseconds()) / 1e9;
#else
  // Microsecond resolution
  return static_cast<double>(duration.total_microseconds()) / 1e6;
#endif
}

//-----------------------------------------------------------------------------------------------
/**
 * Return a time_duration object with the given the number of seconds
 */
time_duration duration_from_seconds(double duration)
{

#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  // Nanosecond resolution
  long fracsecs = long ( 1e9 * fmod(duration, 1.0) );
  long secs = static_cast<long>(  duration  );
  return boost::posix_time::time_duration(0,0,secs, fracsecs);
#else
  // Microsecond resolution
  long fracsecs = long ( 1e6 * fmod(duration, 1.0) );
  long secs = static_cast<long>(  duration  );
  return boost::posix_time::time_duration(0,0,secs, fracsecs);
#endif

}

//-----------------------------------------------------------------------------------------------
/// Create dateAndTime instance from a ISO 8601 yyyy-mm-ddThh:mm:ss[Z+-]tz:tz input string
dateAndTime create_DateAndTime_FromISO8601_String(const std::string& str)
{
  //Make a copy
  std::string time = str;

  //Default of no timezone offset
  bool positive_offset = true;
  time_duration tz_offset = boost::posix_time::seconds(0);

  //Replace "T" with a space
  size_t n = time.find('T');
  if (n != std::string::npos)
  {
    //Take out the T, for later
    time[n] = ' ';

    //Adjust for time zones. Fun!
    //Look for the time zone marker
    size_t n2;
    n2 = time.find('Z', n);
    if (n2 != std::string::npos)
    {
      //Found a Z. Remove it, and no timezone fix
      time = time.substr(0, n2);
    }
    else
    {
      //Look for a + or - indicating time zone offset
      size_t n_plus,n_minus,n5;
      n_plus = time.find('+', n);
      n_minus = time.find('-', n);
      if ((n_plus != std::string::npos) || (n_minus != std::string::npos))
      {
        //Either a - or a + was found
        if (n_plus != std::string::npos)
        {
          positive_offset = true;
          n5 = n_plus;
        }
        else
        {
          positive_offset = false;
          n5 = n_minus;
        }

        //Now, parse the offset time
        std::string offset_str = time.substr(n5+1, time.size()-n5-1);

        //Take out the offset from time string
        time = time.substr(0, n5);

        //Separate into minutes and hours
        size_t n6;
        std::string hours_str("0"), minutes_str("0");
        n6 = offset_str.find(':');
        if ((n6 != std::string::npos))
        {
          //Yes, minutes offset are specified
          minutes_str = offset_str.substr(n6+1, offset_str.size()-n6-1);
          hours_str = offset_str.substr(0, n6);
        }
        else
          //Just hours
          hours_str = offset_str;

        //Convert to a time_duration
        tz_offset = boost::posix_time::hours( boost::lexical_cast<long>(hours_str)) +
            boost::posix_time::minutes( boost::lexical_cast<long>(minutes_str));

      }
    }

  }


  //The boost conversion will convert the string, then we subtract the time zone offset
  if (positive_offset)
    //The timezone is + so we need to subtract the hours
    return boost::posix_time::time_from_string(time) - tz_offset;
  else
    //The timezone is - so we need to ADD the hours
    return boost::posix_time::time_from_string(time) + tz_offset;

}

//-----------------------------------------------------------------------------------------------
/// Create a ISO 8601 yyyy-mm-ddThh:mm:ss string from a time
std::string create_ISO8601_String(const dateAndTime &time)
{
  return to_string(time, "%Y-%m-%dT%H:%M:%S");
}


//-----------------------------------------------------------------------------------------------
/** Convert a dateAndTime object to a std::tm time structure, using whatever time zone in the
 * dateAndtime (should be UTC) .
 */
std::tm to_tm(const dateAndTime &time)
{
  std::tm as_tm;
  try
  {
    as_tm = boost::posix_time::to_tm(time);
  } catch ( std::out_of_range & )
  { // MW 26/10 - above code throws on some setups, create "dummy" date object
    as_tm.tm_year = 1901;
    as_tm.tm_mon = 0;
    as_tm.tm_mday = 1;
    as_tm.tm_hour = 0;
    as_tm.tm_min = 0;
    as_tm.tm_sec = 0;
  }
  return as_tm;
}

//-----------------------------------------------------------------------------------------------
/** Convert a dateAndTime object (in UTC) to a std::tm time structure, using the locat time zone.
 */
std::tm to_localtime_tm(const dateAndTime &time)
{
  //Get the time_t in UTC
  std::time_t my_time_t = to_time_t(time);
  std::tm result;

#ifdef _WIN32
  //Windows version
  localtime_s(&result, &my_time_t);
#else
  //Unix implementation
  localtime_r(&my_time_t, &result);
#endif

  return result;
}


//-----------------------------------------------------------------------------------------------
/** Returns the dateAndTime, in UTC time, corresponding to the pulse time
 * (which is specified in nanoseconds since Jan 1, 1990)
 */
dateAndTime get_time_from_pulse_time(const PulseTimeType& pulse)
{
  PulseTimeType sec = pulse / 1000000000;
  PulseTimeType nanosec = pulse % 1000000000;

#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  // Nanosecond resolution
  boost::posix_time::time_duration td(0,0, sec, nanosec);
#else
  // Microsecond resolution
  boost::posix_time::time_duration td(0,0, sec, nanosec/1000);
#endif

  return GPS_EPOCH + td;

//  return GPS_EPOCH + boost::posix_time::seconds(sec) + boost::posix_time::nanoseconds(nanosec);
}


/** Return the pulse time, as PulseTimeType, from an absolute time given.
 */
PulseTimeType get_from_absolute_time(dateAndTime time)
{
  //Our reference is the GPS epoch.
  boost::posix_time::time_duration td = time - GPS_EPOCH;

#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  // Nanosecond resolution
  PulseTimeType nanosec = td.total_nanoseconds();
  return nanosec;
#else
  // Microsecond resolution
  PulseTimeType nanosec = td.total_microseconds() * 1000;
#endif
  //Total in nanoseconds since 1990.
  return nanosec;
}

/// Returns the maximum value the PulseTime can take.
PulseTimeType getMaximumPulseTime()
{
  //return std::numeric_limits<int64_t>::max()-1;
  //return std::numeric_limits<int64_t>::max();
  return static_cast<int64_t>( 1e18 );
}

/// Returns the minimum value the PulseTime can take.
PulseTimeType getMinimumPulseTime()
{
  //return std::numeric_limits<int64_t>::min()+1;
  return static_cast<int64_t>( -1e18 );
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

//-----------------------------------------------------------------------------------------------
/** Convert a dateAndTime object (in UTC time) to std::time_t, in the LOCAL timezone.
 */
std::time_t to_localtime_t(const dateAndTime &time)
{
  std::tm as_tm = boost::posix_time::to_tm(time);
  std::time_t as_time_t = mktime( &as_tm );
  return as_time_t;
}

//-----------------------------------------------------------------------------------------------
/** Return time as string in simple format CCYY-mmm-dd hh:mm:ss
 */
std::string to_simple_string(const dateAndTime &time)
{
  return boost::posix_time::to_simple_string(time);
}

//-----------------------------------------------------------------------------------------------
/** Return time as string in the specified format, using strftime().
 * Note: Can't do fractional seconds!
 * @param time dateAndTime to output
 * @param format string for the format, e.g. "%Y-%m-%d %H:%M:%S"
 */
std::string to_string(const dateAndTime &time, const char *format)
{
  char buffer [100]; //max 100 characters in the format
  std::tm time_tm = boost::posix_time::to_tm(time); //turn into that struct
  strftime (buffer, 100, format, &time_tm); //Make into a string
  return std::string(buffer);
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
}

/// String representation of the end time
std::string TimeInterval::end_str()const
{
  return boost::posix_time::to_simple_string(this->m_end);
}


} // namespace Kernel
} // namespace Mantid

std::ostream& operator<<(std::ostream& s,const Mantid::Kernel::TimeInterval& t)
{
  s << t.begin() << " - " << t.end();
  return s;
}
