#ifndef DATE_AND_TIME_H
#define DATE_AND_TIME_H

#include "MantidKernel/System.h"
#include <ctime>
#include <ostream>

namespace Mantid
{
namespace Kernel
{

/// The date-and-time is currently stored as a time_t
typedef std::time_t dateAndTime;

/** Represents a time interval. 

    @author Roman Tolchenov, Tessella plc,
    @date 25/03/2009

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    dateAndTime length()const{return m_end - m_begin;}
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

