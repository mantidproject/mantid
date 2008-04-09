#ifndef MANTID_KERNEL_PROPERTYWITHVALUE_H_
#define MANTID_KERNEL_PROPERTYWITHVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** @class PropertyWithValue PropertyWithValue.h Kernel/PropertyWithValue.h

    The concrete, templated class for properties.
    The supported types at present are int, double, bool & std::string.
    
    With reference to the Gaudi structure, this class can be seen as the equivalent of both the
    Gaudi class of the same name and its sub-classses.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 14/11/2007
    
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
template <typename TYPE>
class PropertyWithValue : public Property
{
private:

  /** Private helper class for the PropertyWithValue::setValue & value methods.
      Avoids construction of lexical_cast for types with which it isn't compatible.
   */
  template <class T>
  struct PropertyUtility
  {
    /** Performs the lexical_cast for the PropertyWithValue::value method
     *  @param value The value of the property to be converted to a string
     */
    std::string value(const T& value) const
    {
      return boost::lexical_cast<std::string>( value );
    }
    
    /** Performs the lexical_cast for the PropertyWithValue::setValue method
     *  @param value The value to assign to the property
     *  @param result The result of the lexical_cast
     */
    void setValue(const std::string& value, T& result)
    {
      result = boost::lexical_cast<T>( value );
    }
  };

  /// Specialisation for WorkspaceProperty.
  template <class T>
  struct PropertyUtility<boost::shared_ptr<T> >
  {
    /// Should never get called. Just throws.
    std::string value(const boost::shared_ptr<T>& value) const
    {
      throw boost::bad_lexical_cast();
    }

    /// Should never get called. Just throws.
    void setValue(const std::string& value, boost::shared_ptr<T>& result)
    {
      throw boost::bad_lexical_cast();
    }
  };

  /// Specialisation for a property of type std::vector (as in an ArrayProperty)
  template <class T>
  struct PropertyUtility<std::vector<T> >
  {
    /// Converts the vector of values to a comma-separated string representation
    std::string value(const std::vector<T>& value) const
    {
      std::stringstream s;
      for (unsigned int i = 0; i < value.size(); ++i)
      {
        s << value[i];
        if (i != (value.size()-1) ) s << ',';
      }
      return s.str();
    }

    /// Takes a comma-separated string of values and stores them as the vector of values
    void setValue(const std::string& value, std::vector<T>& result)
    {
      // Split up comma-separated properties
      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      
      boost::char_separator<char> sep(",");
      tokenizer values(value, sep);
      std::vector<T> vec;
      for (tokenizer::iterator it = values.begin(); it != values.end(); ++it)
      {
        try
        {
          vec.push_back( boost::lexical_cast<T>( *it ) );
        }
        catch (boost::bad_lexical_cast e)
        {
          g_log.error("Attempt to set value with mis-formed string for this property type");
          throw;
        }
      }
      if (vec.size() != result.size())
      {
        g_log.information("New property value has different number of elements");
      }
      result = vec;
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
    m_validator( right.m_validator->clone() )
  {  
  }
	
  /// Virtual destructor
  virtual ~PropertyWithValue()
  {
    delete m_validator;
  }
	
  /** Get the value of the property as a string
   *  @return The property's value
   */
  virtual std::string value() const
  {
    PropertyUtility<TYPE> helper;
    return helper.value(m_value);
  }
	
  /** Set the value of the property from a string representation.
   *  Note that "1" & "0" must be used for bool properties rather than true/false.
   *  @param value The value to assign to the property
   *  @return True if the assignment was successful
   */
  virtual bool setValue( const std::string& value )
  {
    try
    {
      PropertyUtility<TYPE> helper;
      TYPE result = m_value;
      helper.setValue(value, result);
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
    m_validator = right.m_validator->clone();
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
  virtual const bool isValid() const
  {
    return m_validator->isValid(m_value);
  }

protected:
  /// The value of the property
  mutable TYPE m_value;  // mutable so that it can be set in WorkspaceProperty::isValid() method
  
private:
  /// Visitor validator class
  IValidator<TYPE> *m_validator;

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
  
  /// Private default constructor
  PropertyWithValue();
};

template <typename TYPE>
Kernel::Logger& PropertyWithValue<TYPE>::g_log = Kernel::Logger::get("PropertyWithValue");

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYWITHVALUE_H_*/
