#ifndef TIMESERIESPROPERTY_H_
#define TIMESERIESPROPERTY_H_

#include "Property.h"
#include "Exception.h"
#include <map>
#include "boost/date_time/posix_time/posix_time.hpp"

namespace Mantid
{
namespace Kernel
{

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
	
	// Property method
	std::string value() const
	{
	  throw Exception::NotImplementedError("Not yet");
	}
	
	// Property method
	bool setValue( const std::string& value )
	{
	  throw Exception::NotImplementedError("Not yet");
	}
	
	// Add a value to the map
	bool addValue( std::string &time, TYPE &value )
	{
    return m_propertySeries.insert( typename std::map<dateAndTime, TYPE>::value_type( 

      dateAndTime(boost::posix_time::from_iso_string(time.c_str())), value) ).second;
	}

  /// for testing that values stored ok - while debugging
  void printMapToScreen() 
  { 
    typename std::map<dateAndTime, TYPE>::iterator p = m_propertySeries.begin();

    while ( p != m_propertySeries.end() )
    {
      std::cout << p->first << "  " << p->second << std::endl;
      p++;
    }

  }
	
private:
  std::map<dateAndTime, TYPE> m_propertySeries;
  
  /// Private default constructor
  TimeSeriesProperty();
};

} // namespace Kernel
} // namespace Mantid

#endif /*TIMESERIESPROPERTY_H_*/
