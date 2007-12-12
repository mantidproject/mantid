#ifndef TIMESERIESPROPERTY_H_
#define TIMESERIESPROPERTY_H_

#include "Property.h"
#include "Exception.h"
#include <map>
#include "boost/date_time/posix_time/posix_time.hpp"
//#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace Kernel
{
/** @class TimeSeriesProperty TimeSeriesProperty.h Kernel/TimeSeriesProperty.h

    A specialised Property class for holding a series of time-value pairs.
    Required by the LoadLog class.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 26/11/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 12/12/2007

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
template <typename TYPE>
class TimeSeriesProperty : public Property
{
public:
  /// The date-and-time will be stored as the boost ptime type
  typedef boost::posix_time::ptime dateAndTime;

  /** Constructor
   *  @param name The name to assign to the property
   */
	explicit TimeSeriesProperty( const std::string &name ) :
	  Property( name, typeid( std::map<dateAndTime, TYPE> ) ),
	  m_propertySeries()
	{
	}
	
	/// Virtual destructor
	virtual ~TimeSeriesProperty() {}
	
	/* Overwrite Property method
   *
   * @return time series property as a string
   */
	std::string value() const	
  {
    std::stringstream ins;

    try {
      typename std::map<dateAndTime, TYPE>::const_iterator p = m_propertySeries.begin();

      while ( p != m_propertySeries.end() )
      {
        ins << p->first << "  " << p->second << std::endl;
        p++;
	    }

      return ins.str();
    }
    catch ( boost::bad_lexical_cast e ) 
    {
     // g_log.error() << "Casting error in TimeSeriesProperty." << e.what();
      return std::string(""); 
	  }
	}
	

	// Overwrite Property method
	bool setValue( const std::string& value )
	{
	  throw Exception::NotImplementedError("Not yet");
	}
	
	/** Add a value to the map
	 *  @param time The time as a string
	 *  @param value The associated value
	 *  @return True if insertion successful (i.e. identical time not already in map
	 */
	bool addValue( const std::string &time, const TYPE value )
	{
	  try {
      return m_propertySeries.insert( typename std::map<dateAndTime, TYPE>::value_type( 
               dateAndTime(boost::posix_time::from_iso_string(time.c_str())), value) ).second;
	  } catch ( boost::bad_lexical_cast e ) {
	    return false;
	  }
	}

	
private:
  /// Holds the time series data
  std::map<dateAndTime, TYPE> m_propertySeries;
  
  /// Private default constructor
  TimeSeriesProperty();

  /// static reference to the logger class
  //static Kernel::Logger& g_log;
};

} // namespace Kernel
} // namespace Mantid

#endif /*TIMESERIESPROPERTY_H_*/
