#ifndef MANTID_KERNEL_PROPERTYWITHVALUE_H_
#define MANTID_KERNEL_PROPERTYWITHVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_pointer.hpp>

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
private:
  /// Private helper class for the PropertyWithValue::setValue method
  struct PropertyUtility
  {
    /// Required for compilation of WorkspaceProperty. Should never get called.
    template <typename T>
    void setValue(const std::string& value, T& result, const boost::true_type&)
    {
      // As this method should never get called, just throw a bad_lexical_cast
      throw boost::bad_lexical_cast();
    }
    
    /** Performs the lexical_cast for the PropertyWithValue::setValue method
     *  @param value The value to assign to the property
     *  @param result The result of the lexical_cast
     */
    template <typename T>
    void setValue(const std::string& value, T& result, const boost::false_type&)
    {
      result = boost::lexical_cast<T>( value );
    }
  };
  
//----------------------------------------------------------------------
// Now the PropertyWithValue class itself
//----------------------------------------------------------------------
public:
  /** Constructor
   *  @param name The name to assign to the property
   *  @param value The initial value to assign to the property
   *  @param validator The validator to use for this property (this class will take ownership of the validator)
   */
  PropertyWithValue( const std::string &name, TYPE value, IValidator<TYPE> *validator = new NullValidator<TYPE> ) :
    Property( name, typeid( TYPE ) ),
    m_value( value ),
    m_validator( validator )
  {
  }

  /// Copy constructor
  PropertyWithValue( const PropertyWithValue& right ) :
    Property( right ),
    m_value( right.m_value ),
    m_validator( right.m_validator )
  {  
  }
	
  /// Virtual destructor
  virtual ~PropertyWithValue()
  {
  }
	
  /** Get the value of the property as a string
   *  @return The property's value
   */
  virtual std::string value() const
  {
    // Could potentially throw a bad_lexical_cast, but that shouldn't happen with the types we're supporting
    return boost::lexical_cast<std::string>( m_value );
  }
	
  /** Set the value of the property from a string representation
   *  @param value The value to assign to the property
   *  @return True if the assignment was successful
   */
  virtual bool setValue( const std::string& value )
  {
    try
    {
      PropertyUtility helper;
      TYPE result;
      helper.setValue(value, result, boost::is_pointer<TYPE>());
      // Use the assignment operator defined below
      *this = result;
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
    m_value = right.m_value;
    m_isDefault = false;
    return *this;
  }
	
  /** Assignment operator.
   *  Allows assignment of a new value to the property by writing,
   *  e.g., myProperty = 3;
   *  @param value The new value to assign to the property
   */
  virtual TYPE& operator=( const TYPE& value )
  {
    m_value = value;
    m_isDefault = false;
    return m_value;
  }

  /// Allows you to get the value of the property via an expression like myProperty()
  virtual const TYPE& operator() () const
  {
    return m_value;
  }

  /** Allows you to get the value of the property simply by typing its name.
   *  Means you can use an expression like: int i = myProperty;
   */
  virtual operator const TYPE& () const
  {
    return m_value;
  }

  /** Checks if the value is valid for this property.
   *  @returns true if the value is valid, otherwise false.
   */
  virtual const bool isValid()
  {
    return m_validator->isValid(m_value);
  }

private:
  /// The value of the property
  TYPE m_value;
  /// Visitor validator class
  IValidator<TYPE> *m_validator;
  
  /// Private default constructor
  PropertyWithValue();
};


} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYWITHVALUE_H_*/
