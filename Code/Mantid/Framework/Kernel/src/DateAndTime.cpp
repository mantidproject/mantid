#include "MantidKernel/DateAndTime.h"
#include <time.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeParser.h>
#include <boost/date_time/posix_time/posix_time_config.hpp>

namespace Mantid
{
namespace Kernel
{


/// Max allowed nanoseconds in the time; 2^62-1
static int64_t MAX_NANOSECONDS = 4611686018427387903LL;

/// Max allowed seconds in the time
static int64_t MAX_SECONDS      = 4611686017LL;

/// Min allowed nanoseconds in the time; -2^62+1
static int64_t MIN_NANOSECONDS  = -4611686018427387903LL;

/// Min allowed seconds in the time
static int64_t MIN_SECONDS      = -4611686017LL;


namespace DateAndTimeHelpers
{

//-----------------------------------------------------------------------------------------------
/** Convert time_t to tm as UTC time.
 * Portable implementation of gmtime_r (re-entrant gmtime) that works on Windows and Linux
 *
 * @param clock :: pointer to time_t to convert
 * @param result :: pointer to a struct tm (timeinfo structure) that will be filled.
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
    tmp.tm_mon +=  utctime->tm_mon - check.tm_mon;
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




} //namespace DateAndTimeHelpers






//------------------------------------------------------------------------------------------------
/** Default, empty constructor */
DateAndTime::DateAndTime() : _nanoseconds(0)
{}

//------------------------------------------------------------------------------------------------
/** Construct a date from nanoseconds.
 * @param total_nanoseconds :: nanoseconds since Jan 1, 1990 (our epoch).
 */
DateAndTime::DateAndTime(const int64_t total_nanoseconds)
{
  //Make sure that you cannot construct a date that is beyond the limits...
  if (total_nanoseconds > MAX_NANOSECONDS)
    _nanoseconds = MAX_NANOSECONDS;
  else if (total_nanoseconds < MIN_NANOSECONDS)
    _nanoseconds = MIN_NANOSECONDS;
  else
    _nanoseconds = total_nanoseconds;
}

//------------------------------------------------------------------------------------------------
/** Construct a time from an ISO8601 string
 *
 * @param ISO8601_string: and ISO8601 formatted string.
 *    "yyyy-mm-ddThh:mm:ss[Z+-]tz:tz"; although the T can be replaced by a space,
 *    and the time is optional, as is the time-zone specification.
 */
DateAndTime::DateAndTime(const std::string ISO8601_string) : _nanoseconds(0)
{
  this->set_from_ISO8601_string(ISO8601_string);
}

//------------------------------------------------------------------------------------------------
/** Construct time from a boost::posix_time::ptime.
 * @param _ptime :: boost::posix_time::ptime
 */
DateAndTime::DateAndTime(const boost::posix_time::ptime _ptime) : _nanoseconds(0)
{
  this->set_from_ptime(_ptime);
}

//------------------------------------------------------------------------------------------------
/** Construct a time from the number of seconds and nanoseconds since Jan 1, 1990.
 * @param seconds :: seconds since Jan 1, 1990.
 * @param nanoseconds :: nanoseconds to add to the number of seconds
 */
DateAndTime::DateAndTime(const double seconds, const double nanoseconds)
{
  double nano = seconds * 1e9 + nanoseconds;
  //Limit times
  if (nano > MAX_NANOSECONDS)
    _nanoseconds = MAX_NANOSECONDS;
  else if (nano < MIN_NANOSECONDS)
    _nanoseconds = MIN_NANOSECONDS;
  else
    _nanoseconds = static_cast<int64_t>(nano);
}

//------------------------------------------------------------------------------------------------

/** Construct a time from the number of seconds and nanoseconds since Jan 1, 1990.
 * @param seconds :: seconds since Jan 1, 1990.
 * @param nanoseconds :: nanoseconds to add to the number of seconds
 */
DateAndTime::DateAndTime(const int64_t seconds, const int64_t nanoseconds)
{
  if (seconds >= MAX_SECONDS)
    _nanoseconds = MAX_NANOSECONDS;
  else if (seconds <= MIN_SECONDS)
    _nanoseconds = MIN_NANOSECONDS;
  else
    _nanoseconds = static_cast<int64_t>(seconds * 1000000000LL + nanoseconds);
}

//------------------------------------------------------------------------------------------------

/** Construct a time from the number of seconds and nanoseconds since Jan 1, 1990.
 * @param seconds :: seconds since Jan 1, 1990.
 * @param nanoseconds :: nanoseconds to add to the number of seconds
 */
DateAndTime::DateAndTime(const int32_t seconds, const int32_t nanoseconds)
{
  if (seconds >= MAX_SECONDS)
    _nanoseconds = MAX_NANOSECONDS;
  else if (seconds <= MIN_SECONDS)
    _nanoseconds = MIN_NANOSECONDS;
  else
    _nanoseconds = static_cast<int64_t>(seconds * 1000000000LL + nanoseconds);
}

//===========================================================================================
//===========================================================================================

//------------------------------------------------------------------------------------------------
/** Get the time as a boost::posix_time::ptime.
 * @return a boost::posix_time::ptime.
 */
boost::posix_time::ptime DateAndTime::to_ptime() const
{
  return DateAndTimeHelpers::GPS_EPOCH + duration_from_nanoseconds(_nanoseconds);
}


//------------------------------------------------------------------------------------------------
/** Sets the date and time using a boost::posix_time::ptime
 *
 * @param _ptime :: boost::posix_time::ptime date and time.
 */
void DateAndTime::set_from_ptime(boost::posix_time::ptime _ptime)
{
  if (_ptime.is_special())
  {
    // --- SPECIAL VALUES! ----
    if (_ptime.is_infinity() || _ptime.is_pos_infinity())
      _nanoseconds = MAX_NANOSECONDS;
    if (_ptime.is_neg_infinity())
      _nanoseconds = MIN_NANOSECONDS;
    if (_ptime.is_not_a_date_time())
      _nanoseconds = MIN_NANOSECONDS;
  }
  else
  {
    _nanoseconds = nanoseconds_from_duration(_ptime - DateAndTimeHelpers::GPS_EPOCH);

    //Check for overflow
    if (_nanoseconds < 0)
    {
      if (_ptime.date().year() >= 1990)
      {
        //nanoseconds is negative despite the year being higher than 1990
        // ... means overflow occured
        this->set_to_maximum();
      }
    }
    else if (_nanoseconds > 0)
    {
      if (_ptime.date().year() < 1990)
      {
        //Nanoseconds is positive but the year is below 1990 = it should be negative!
        this->set_to_minimum();
      }
    }
  }

}


//===========================================================================================
//========================== time_t support =================================================
//===========================================================================================
//------------------------------------------------------------------------------------------------
/** Sets the date and time using a std::time_t
 *
 * @param _timet :: std::time_t to set to
 */
void DateAndTime::set_from_time_t(std::time_t _timet)
{
  this->set_from_ptime( boost::posix_time::from_time_t( _timet ) );
}

//------------------------------------------------------------------------------------------------
/** Get the time as a std::time_t
 * @return a std::time_t
 */
std::time_t DateAndTime::to_time_t() const
{
  std::tm as_tm = boost::posix_time::to_tm(this->to_ptime());
  std::time_t to_time_t = DateAndTimeHelpers::utc_mktime( &as_tm );
  return to_time_t;
}


//-----------------------------------------------------------------------------------------------
/** Convert a DateAndTime object to a std::tm time structure, using whatever time zone in the
 * dateAndtime (should be UTC) .
 */
std::tm DateAndTime::to_tm() const
{
  std::tm as_tm;
  try
  {
    as_tm = boost::posix_time::to_tm(this->to_ptime());
  } catch ( std::out_of_range & )
  { // MW 26/10 - above code throws on some setups, create "dummy" date object
    as_tm.tm_year = 0;
    as_tm.tm_mon = 0;
    as_tm.tm_mday = 1;
    as_tm.tm_hour = 0;
    as_tm.tm_min = 0;
    as_tm.tm_sec = 0;
    as_tm.tm_wday = 0;
    as_tm.tm_yday = 0;
    as_tm.tm_isdst = 0;
  }
  return as_tm;
}

//-----------------------------------------------------------------------------------------------
/** Convert a DateAndTime object (in UTC) to a std::tm time structure, using the locat time zone.
 */
std::tm DateAndTime::to_localtime_tm() const
{
  //Get the time_t in UTC
  std::time_t my_time_t = this->to_time_t();
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
/** Convert a DateAndTime object (in UTC time) to std::time_t, in the LOCAL timezone.
 */
std::time_t DateAndTime::to_localtime_t() const
{
  std::tm as_tm = this->to_tm();
  std::time_t to_time_t = mktime( &as_tm );
  return to_time_t;
}


//------------------------------------------------------------------------------------------------
/** Sets the date and time to the maximum allowed value */
void DateAndTime::set_to_maximum()
{
  _nanoseconds = MAX_NANOSECONDS; //+2^62, or around the year 2136
}

/** Sets the date and time to the minimum allowed value */
void DateAndTime::set_to_minimum()
{
  _nanoseconds = MIN_NANOSECONDS; //-2^62, or around the year 1843
}

/** Return the maximum time possible */
DateAndTime DateAndTime::maximum()
{
  return DateAndTime(MAX_NANOSECONDS);
}

/** Return the minimum time possible */
DateAndTime DateAndTime::minimum()
{
  return DateAndTime(MIN_NANOSECONDS);
}

/// A default date and time to use when time is not specified
const DateAndTime DateAndTime::defaultTime()
{
  return DateAndTime("1970-01-01T00:00:00");
}


//------------------------------------------------------------------------------------------------
/** Sets the date and time using an ISO8601-formatted string
 *
 * @param str :: ISO8601 format string: "yyyy-mm-ddThh:mm:ss[Z+-]tz:tz"
 */
void DateAndTime::set_from_ISO8601_string(const std::string str)
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
    this->set_from_ptime( boost::posix_time::time_from_string(time) - tz_offset );
  else
    //The timezone is - so we need to ADD the hours
    this->set_from_ptime( boost::posix_time::time_from_string(time) + tz_offset );
}


//------------------------------------------------------------------------------------------------
/** Return the date and time as a simple string,
 * for example, "19-Feb-2010 11:23:34.456000000"
 * @return date-time formatted as a simple string
 */
std::string DateAndTime::to_simple_string() const
{
  return boost::posix_time::to_simple_string(this->to_ptime());
}


//------------------------------------------------------------------------------------------------
/** Return the date and time as a string, using formatting of strftime().
 * Default format gives "1990-Jan-02 03:04:05"
 * @param format : format for strftime(). Default "%Y-%b-%d %H:%M:%S"
 * @return date as string, formatted as requested
 */
std::string DateAndTime::to_string(const std::string format) const
{
  char buffer [25];
  std::tm date_as_tm = this->to_tm();
  strftime (buffer, 25, format.c_str(), &date_as_tm);
  return std::string(buffer);
}

//------------------------------------------------------------------------------------------------
/** Return the date and time as an ISO8601-formatted string without fractional seconds.
 * @return string
 */
std::string DateAndTime::to_ISO8601_string() const
{
  return this->to_string("%Y-%m-%dT%H:%M:%S");
}


////------------------------------------------------------------------------------------------------
///** Stream operator
// */
//std::ostream& DateAndTime::operator<< (std::ostream& stream, const DateAndTime & obj)
//{
//  stream << obj->to_simple_string();
//  return stream;
//}

//------------------------------------------------------------------------------------------------
/** Get the year of this date.
 * @return the year
 */
int DateAndTime::year() const
{
  return to_ptime().date().year();
}

/** Get the month of this date.
 * @return the month
 */
int DateAndTime::month() const
{
  return to_ptime().date().month();
}

/** Get the day (of the month) of this date.
 * @return the day
 */
int DateAndTime::day() const
{
  return to_ptime().date().day();
}

/** Get the hour (0-24) of this time.
 * @return the hour
 */
int DateAndTime::hour() const
{
  return to_ptime().time_of_day().hours();
}

/** Get the minute (0-60) of this time.
 * @return the minute
 */
int DateAndTime::minute() const
{
  return to_ptime().time_of_day().minutes();
}

/** Get the seconds (0-60) of this time.
 * @return the second
 */
int DateAndTime::second() const
{
  return to_ptime().time_of_day().seconds();
}

/** Get the nanoseconds (remainder, < 1 second) of this time.
 * @return the nanoseconds
 */
int DateAndTime::nanoseconds() const
{
  return static_cast<int>(_nanoseconds % 1000000000);
}

//------------------------------------------------------------------------------------------------
/** Return the total # of nanoseconds since the epoch */
int64_t DateAndTime::total_nanoseconds() const
{
  return this->_nanoseconds;
}


//------------------------------------------------------------------------------------------------
/** == operator
 * @param rhs :: DateAndTime to compare
 * @return true if equals
 */
bool DateAndTime::operator==(const DateAndTime& rhs) const
{
  return _nanoseconds == rhs._nanoseconds;
}

/** == operator for boost::posix_time::ptime
 * @param rhs :: boost::posix_time::ptime to compare
 * @return true if equals
 */
bool DateAndTime::operator==(const boost::posix_time::ptime& rhs) const
{
  return this->to_ptime() == rhs;
}


/** != operator
 * @param rhs :: DateAndTime to compare
 * @return true if not equals
 */
bool DateAndTime::operator!=(const DateAndTime& rhs) const
{
  return _nanoseconds != rhs._nanoseconds;
}

/** < operator
 * @param rhs :: DateAndTime to compare
 * @return true if less than
 */
bool DateAndTime::operator<(const DateAndTime& rhs) const
{
  return _nanoseconds < rhs._nanoseconds;
}

/** <= operator
 * @param rhs :: DateAndTime to compare
 * @return true if less than or equals
 */
bool DateAndTime::operator<=(const DateAndTime& rhs) const
{
  return _nanoseconds <= rhs._nanoseconds;
}

/** > operator
 * @param rhs :: DateAndTime to compare
 * @return true if greater than
 */
bool DateAndTime::operator>(const DateAndTime& rhs) const
{
  return _nanoseconds > rhs._nanoseconds;
}

/** >= operator
 * @param rhs :: DateAndTime to compare
 * @return true if greater than or equals
 */
bool DateAndTime::operator>=(const DateAndTime& rhs) const
{
  return _nanoseconds >= rhs._nanoseconds;
}



//------------------------------------------------------------------------------------------------
/** + operator to add time.
 * @param nanosec :: number of nanoseconds to add
 * @return modified DateAndTime.
 */
DateAndTime DateAndTime::operator+(const int64_t nanosec) const
{
  return DateAndTime(_nanoseconds + nanosec);
}

/** += operator to add time.
 * @param nanosec :: number of nanoseconds to add
 * @return modified DateAndTime.
 */
DateAndTime& DateAndTime::operator+=(const int64_t nanosec)
{
  _nanoseconds += nanosec;
  if (_nanoseconds > MAX_NANOSECONDS) _nanoseconds = MAX_NANOSECONDS;
  else if (_nanoseconds < MIN_NANOSECONDS) _nanoseconds = MIN_NANOSECONDS;
  return *this;
}

/** - operator to subtract time.
 * @param nanosec :: number of nanoseconds to subtract
 * @return modified DateAndTime.
 */
DateAndTime DateAndTime::operator-(const int64_t nanosec) const
{
  return DateAndTime(_nanoseconds - nanosec);
}

/** -= operator to subtract time.
 * @param nanosec :: number of nanoseconds to subtract
 * @return modified DateAndTime.
 */
DateAndTime& DateAndTime::operator-=(const int64_t nanosec)
{
  _nanoseconds -= nanosec;
  if (_nanoseconds > MAX_NANOSECONDS) _nanoseconds = MAX_NANOSECONDS;
  else if (_nanoseconds < MIN_NANOSECONDS) _nanoseconds = MIN_NANOSECONDS;
  return *this;
}


//------------------------------------------------------------------------------------------------
/** + operator to add time.
 * @param td :: time_duration to add
 * @return modified DateAndTime.
 */
DateAndTime DateAndTime::operator+(const time_duration& td) const
{
  return this->operator +(nanoseconds_from_duration(td));
}

/** += operator to add time.
 * @param td :: time_duration to add
 * @return modified DateAndTime.
 */
DateAndTime& DateAndTime::operator+=(const time_duration& td)
{
  return this->operator +=(nanoseconds_from_duration(td));
}

/** - operator to subtract time.
 * @param td :: time_duration to subtract
 * @return modified DateAndTime.
 */
DateAndTime DateAndTime::operator-(const time_duration& td) const
{
  return this->operator -(nanoseconds_from_duration(td));
}

/** -= operator to subtract time.
 * @param td :: time_duration to subtract
 * @return modified DateAndTime.
 */
DateAndTime& DateAndTime::operator-=(const time_duration& td)
{
  return this->operator -=(nanoseconds_from_duration(td));
}


//------------------------------------------------------------------------------------------------
/** + operator to add time.
 * @param sec :: duration to add
 * @return modified DateAndTime.
 */
DateAndTime DateAndTime::operator+(const double sec) const
{
  return this->operator +(nanoseconds_from_seconds(sec));
}

/** += operator to add time.
 * @param sec :: duration to add
 * @return modified DateAndTime.
 */
DateAndTime& DateAndTime::operator+=(const double sec)
{
  return this->operator +=(nanoseconds_from_seconds(sec));
}

/** - operator to subtract time.
 * @param sec :: duration to subtract
 * @return modified DateAndTime.
 */
DateAndTime DateAndTime::operator-(const double sec) const
{
  return this->operator -(nanoseconds_from_seconds(sec));
}

/** -= operator to subtract time.
 * @param sec :: duration to subtract
 * @return modified DateAndTime.
 */
DateAndTime& DateAndTime::operator-=(const double sec)
{
  return this->operator -=(nanoseconds_from_seconds(sec));
}



//------------------------------------------------------------------------------------------------
/** Subtract two times.
 * @param rhs object on right of operation
 * @return a time_duration
 */
time_duration DateAndTime::operator-(const DateAndTime& rhs) const
{
  return this->to_ptime() - rhs.to_ptime();
}


//------------------------------------------------------------------------------------------------
/** Returns the current DateAndTime, in UTC time, with microsecond precision
 * @return the current time.
 */
DateAndTime DateAndTime::get_current_time()
{
  return DateAndTime(boost::posix_time::microsec_clock::universal_time());
}



//-----------------------------------------------------------------------------------------------
/**
 * Return the number of seconds in a time_duration, as a double, including fractional seconds.
 */
double DateAndTime::seconds_from_duration(time_duration duration)
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
time_duration DateAndTime::duration_from_seconds(double duration)
{
  long secs = static_cast<long>(  duration  );

  //Limit the seconds to the range of long (avoid overflows)
  if (duration >= std::numeric_limits<int>::max())
    return boost::posix_time::time_duration( boost::posix_time::max_date_time );
  else if (duration <= std::numeric_limits<int>::min())
    return boost::posix_time::time_duration( boost::posix_time::min_date_time );

  typedef boost::posix_time::time_res_traits::sec_type sec_type;

#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  // Nanosecond resolution
  long fracsecs = long ( 1e9 * fmod(duration, 1.0) );
  return boost::posix_time::time_duration(0,0,static_cast<sec_type>(secs), fracsecs);
#else
  // Microsecond resolution
  long fracsecs = long ( 1e6 * fmod(duration, 1.0) );
  return boost::posix_time::time_duration(0,0,static_cast<sec_type>(secs), fracsecs);
#endif

}


//-----------------------------------------------------------------------------------------------
/** time duration in nanoseconds. Duration is limited to
 * MAX_NANOSECONDS and MIN_NANOSECONDS to avoid overflows.
 * @param td :: time_duration instance.
 * @return an int64 of the number of nanoseconds
 */
int64_t DateAndTime::nanoseconds_from_duration(const time_duration & td)
{
  int64_t nano;
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  // Nanosecond resolution
  nano = td.total_nanoseconds();
#else
  // Microsecond resolution
  nano = (td.total_microseconds()*1000);
#endif
  //Use these limits to avoid integer overflows
  if (nano >MAX_NANOSECONDS)
    return MAX_NANOSECONDS;
  else if (nano < MIN_NANOSECONDS)
    return MIN_NANOSECONDS;
  else
    return nano;
}

//-----------------------------------------------------------------------------------------------
/** time duration from nanoseconds
 * @param dur :: duration in nanoseconds, as an int
 * @return a time_duration type
 */
time_duration DateAndTime::duration_from_nanoseconds(int64_t dur)
{
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
  // Nanosecond resolution
  return boost::posix_time::time_duration(0,0,0,dur);
#else
  // Microsecond resolution
  return boost::posix_time::time_duration(0,0,0,dur/1000);
#endif
}

//-----------------------------------------------------------------------------------------------
/** Nanoseconds from seconds, with limits
 * @param sec :: duration in seconds, as a double
 * @return int64 of the number of nanoseconds
 */
int64_t DateAndTime::nanoseconds_from_seconds(double sec)
{
  double nano = sec * 1e9;
  //Use these limits to avoid integer overflows
  if (nano >MAX_NANOSECONDS)
    return MAX_NANOSECONDS;
  else if (nano < MIN_NANOSECONDS)
    return MIN_NANOSECONDS;
  else
    return int64_t(nano);
}


//-----------------------------------------------------------------------------------------------
/** Static method to create a vector of DateAndTime objects
 * using a start time and seconds offset. To speed things up,
 * no limit checking is performed!
 *
 * @param start :: DateAndTime at 0.
 * @param seconds :: a vector of doubles of the number of seconds.
 * @param out :: reference to a vector that will be filled with DateAndTime's
 */
void DateAndTime::createVector(const DateAndTime start, const std::vector<double> & seconds, std::vector<DateAndTime> & out)
{
  int64_t startnano = start._nanoseconds;
  size_t num = seconds.size();
  out.resize(num);
  std::vector<double>::const_iterator it;
  size_t i = 0;
  for(it = seconds.begin(); it != seconds.end(); it++)
  {
    out[i]._nanoseconds = startnano + static_cast<int64_t>(  (*it) * 1000000000.0 );
    i++;
  }
}

//-----------------------------------------------------------------------------------------------
/** Check if a string is iso8601 format.
 *
 * @param str :: string to check
 * @return true if the string conforms to ISO 860I, false otherwise.
 */
bool DateAndTime::string_isISO8601(const std::string & str)
{
  Poco::DateTime dt;
  int tz_diff;
  return Poco::DateTimeParser::tryParse(Poco::DateTimeFormat::ISO8601_FORMAT, str, dt, tz_diff);
}















TimeInterval::TimeInterval(const DateAndTime& from, const DateAndTime& to)
:m_begin(from)
{
    if (to > from) m_end = to;
    else
        m_end = from;
}

/**  Returns an intersection of this interval with \a ti
     @param ti :: Time interval
     @return A valid time interval if this interval intersects with \a ti or
             an empty interval otherwise.
 */
TimeInterval TimeInterval::intersection(const TimeInterval& ti)const
{
    if (!isValid() || !ti.isValid()) return TimeInterval();

    DateAndTime t1 = begin();
    if (ti.begin() > t1) t1 = ti.begin();

    DateAndTime t2 = end();
    if (ti.end() < t2) t2 = ti.end();

    return t1 < t2? TimeInterval(t1,t2) : TimeInterval();

}

/// String representation of the begin time
std::string TimeInterval::begin_str()const
{
  return boost::posix_time::to_simple_string(this->m_begin.to_ptime());
}

/// String representation of the end time
std::string TimeInterval::end_str()const
{
  return boost::posix_time::to_simple_string(this->m_end.to_ptime());
}


} // namespace Kernel

} // namespace Mantid





