#ifndef LOG_FILTER_H
#define LOG_FILTER_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"

#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Kernel
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class Property;
template<class TYPE> class TimeSeriesProperty;

/** 
    This class is for filtering TimeSeriesProperty data
    
    @author Roman Tolchenov, Tessella plc
    @date 26/11/2007

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
class DLLExport LogFilter
{
public:
    /// Constructor
    LogFilter(const Property* tsp);

    /// Adds a filter using boolean AND
    void addFilter(const TimeSeriesProperty<bool>* filter);

    /** Returns reference to the filtered property. Use its nthValue and nthInterval
        to iterate through the allowed values and time intervals.
        @return A reference to the filtered property
     */
    const TimeSeriesProperty<double>* data()const{return m_prop.get();}

    /// Returns the filter
    const TimeSeriesProperty<bool>* filter()const{return m_filter.get();}

    /// Clears filters
    void clear();
private:
    /// Pointer to the filtered property
    boost::shared_ptr< TimeSeriesProperty<double> > m_prop;

    /// Pointer to the filter mask
    boost::shared_ptr< TimeSeriesProperty<bool> > m_filter;
};



} // namespace Kernel
} // namespace Mantid

#endif // LOG_FILTER_H
