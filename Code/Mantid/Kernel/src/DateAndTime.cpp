#include "MantidKernel/DateAndTime.h"

namespace Mantid
{
namespace Kernel
{

namespace DateAndTime
{

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
