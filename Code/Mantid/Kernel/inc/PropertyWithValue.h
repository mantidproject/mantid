#ifndef MANTID_KERNEL_PROPERTYWITHVALUE_H_
#define MANTID_KERNEL_PROPERTYWITHVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "Property.h"
#include "Exception.h"
#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace Kernel
{
/** @class PropertyWithValue PropertyWithValue.h Kernel/PropertyWithValue.h

    The concrete, templated class for properties.
    The supported types at present are int, double & std::string.
    
    With reference to the Gaudi structure, this class can be seen as the equivalent of both the
    Gaudi class of the same name and its sub-classses.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 14/11/2007
    
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
class PropertyWithValue : public Property
{
public:
  /** Constructor
   *  @param name The name to assign to the property
   *  @param value The initial value to assign to the property
   */
	PropertyWithValue( const std::string &name, TYPE value ) :
	  Property( name, typeid( TYPE ) ),
	  m_value( value )
	{
	}
	
	/// Copy constructor
	PropertyWithValue( const PropertyWithValue& right ) :
	  Property( right ),
	  m_value( right.m_value )
	{  
	}
	
	/// Virtual destructor
	virtual ~PropertyWithValue()
	{
	}
		
	/** Get the value of the property as a string
	 *  @return The property's value
	 */
	std::string value() const
	{
	  // Could potentially throw a bad_lexical_cast, but that shouldn't happen with the types we're supporting
	  return boost::lexical_cast<std::string>( m_value );
	}
		
	/** Set the value of the property from a string representation
	 *  @param value The value to assign to the property
	 *  @return True if the assignment was successful
	 */
	bool setValue( const std::string& value )
	{
	  try
	  {
	    // Use the assignment operator defined below
	    *this = boost::lexical_cast<TYPE>( value );
	    return true;
	  }
	  catch ( boost::bad_lexical_cast )
	  {
	    return false;
	  }
	}
	
	/// Copy assignment operator. Only assigns the value.
	PropertyWithValue& operator=( const PropertyWithValue& right )
	{
	  if ( &right == this ) return *this;
	  // Chain the base class assignment operator for clarity (although it does nothing)
	  Property::operator=( right );
	  // Make use of the assignment operator (below)
	  *this = right;
	  return *this;
	}
	
	/** Assignment operator.
	 *  Allows assignment of a new value to the property by writing,
	 *  e.g., myProperty = 3;
	 *  @param value The new value to assign to the property
	 */
	PropertyWithValue& operator=( const TYPE& value )
	{
	  // Include use of a validator when implemented
	  m_value = value;
	  m_isDefault = false;
	  return *this;
	}
	
	/// Allows you to get the value of the property via an expression like myProperty()
	const TYPE& operator() () const
	{
	  return m_value;
	}
	
	/** Allows you to get the value of the property simply by typing its name. (e.g. myProperty)
	 *  Means you can use an expression like: int i = myProperty;
	 */
	operator const TYPE& () const
  {
    return m_value;
  }
	
private:
  /// The value of the property
  TYPE m_value;
  
  PropertyWithValue();
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYWITHVALUE_H_*/
