#ifndef DATE_AND_TIME_H
#define DATE_AND_TIME_H

#include "MantidKernel/System.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include <ctime>
#include <ostream>

namespace Mantid
{
namespace Kernel
{

/// The date-and-time is currently stored as a boost::posix_time::ptime
typedef boost::posix_time::ptime dateAndTime;

/// Durations and time intervals
typedef boost::posix_time::time_duration time_duration;

/** Typedef of the data structure used to store the pulse times. A signed 64-bit int
 * of the # of milliseconds since Jan 1, 1990.
 */
typedef int64_t PulseTimeType;



namespace DateAndTime
{

/// The difference in seconds between standard unix and gps epochs.
static const uint32_t EPOCH_DIFF = 631152000;

/// The epoch for GPS times.
static const dateAndTime GPS_EPOCH(boost::gregorian::date(1990, 1, 1));

/// The epoch for Unix times.
static const dateAndTime UNIX_EPOCH(boost::gregorian::date(1970, 1, 1));

/// Const of one second time duration
static const time_duration oneSecond = boost::posix_time::time_duration(0,0,1,0);

/// A default date and time to use when time is not specified
static const dateAndTime defaultTime = boost::posix_time::ptime( boost::gregorian::date(1970,1,1) );

DLLExport double durationInSeconds(time_duration duration);

DLLExport time_t utc_mktime(struct tm *utctime);

DLLExport dateAndTime create_DateAndTime_FromISO8601_String(const std::string& str);

DLLExport std::string create_ISO8601_String(const dateAndTime &time);

DLLExport std::string to_simple_string(const dateAndTime &time);

DLLExport std::string to_string(const dateAndTime &time, const char *format);

DLLExport std::time_t to_time_t(const dateAndTime &time);

DLLExport std::time_t to_localtime_t(const dateAndTime &time);

DLLExport dateAndTime from_time_t(const std::time_t &time);

DLLExport std::tm to_tm(const dateAndTime &time);

DLLExport std::tm to_localtime_tm(const dateAndTime &time);

DLLExport dateAndTime get_current_time();

DLLExport dateAndTime get_time_from_pulse_time(const PulseTimeType& pulse);

DLLExport PulseTimeType get_from_absolute_time(dateAndTime time);

}

typedef boost::posix_time::ptime dateAndTime;

/** Represents a time interval. 

    @author Roman Tolchenov, Tessella plc,
    @date 25/03/2009

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport TimeInterval
{
public:
    /// Default constructor
    TimeInterval():m_begin(),m_end(){}
    /// Constructor
    TimeInterval(const dateAndTime& from, const dateAndTime& to);
    /// Beginning of the interval
    dateAndTime begin()const{return m_begin;}
    /// End of the interval
    dateAndTime end()const{return m_end;}
    /// True if the interval is not empty
    bool isValid()const{return m_end > m_begin;}

    /// Interval length (in seconds?)
    time_duration length()const{return m_end - m_begin;}

    /// True if the interval contains \a t.
    bool contains(const dateAndTime& t)const{return t >= begin() && t < end();}
    /// Returns an intersection of two intervals
    TimeInterval intersection(const TimeInterval& ti)const;
    /// Returns true if this interval ends before \a ti starts
    bool operator<(const TimeInterval& ti)const{return end() < ti.begin();}
    /// String representation of the begin time
    std::string begin_str()const;
    /// String representation of the end time
    std::string end_str()const;
private:
    /// begin
    dateAndTime m_begin;
    /// end
    dateAndTime m_end;
};


} // namespace Kernel
} // namespace Mantid

DLLExport std::ostream& operator<<(std::ostream&,const Mantid::Kernel::TimeInterval&);

#endif // DATE_AND_TIME_H

