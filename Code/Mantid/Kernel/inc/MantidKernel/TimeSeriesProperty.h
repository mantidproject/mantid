#ifndef TIMESERIESPROPERTY_H_
#define TIMESERIESPROPERTY_H_

#include "Property.h"
#include "Exception.h"
#include <map>

namespace Mantid
{
namespace Kernel
{

template <typename TYPE>
class TimeSeriesProperty : public Property
{
public:
  /** Constructor
   *  @param name The name to assign to the property
   */
	TimeSeriesProperty( const std::string &name ) :
	  Property( name, typeid( std::map<std::string, TYPE> ) ),
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
	  return m_propertySeries.insert(std::map<std::string, TYPE>::value_type(time, value)).second;
	}
	
private:
  std::map<std::string, TYPE> m_propertySeries;
  
  /// Private default constructor
  TimeSeriesProperty();
};

} // namespace Kernel
} // namespace Mantid

#endif /*TIMESERIESPROPERTY_H_*/
