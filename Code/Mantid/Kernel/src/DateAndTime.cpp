#include "MantidKernel/DateAndTime.h"

namespace Mantid
{
namespace Kernel
{

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
    char buffer [25];
    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&m_begin));
//    strftime (buffer,25,"%H:%M:%S",localtime(&m_begin));
    return std::string(buffer);
}

/// String representation of the end time
std::string TimeInterval::end_str()const
{
    char buffer [25];
    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&m_end));
    return std::string(buffer);
}


} // namespace Kernel
} // namespace Mantid

std::ostream& operator<<(std::ostream& s,const Mantid::Kernel::TimeInterval& t)
{
    char buffer [25];
    Mantid::Kernel::dateAndTime d = t.begin();
    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&d));
    s<<buffer<<" - ";
    d = t.end();
    strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&d));
    s<<buffer;
    return s;
}
