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
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/FileValidator.h"
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
          // remove leading and trailing spaces
          size_t i0 = it->find_first_not_of(" \t");
          size_t i1 = it->find_last_not_of(" \t");
          if (i0 == std::string::npos) throw boost::bad_lexical_cast();
          std::string str = it->substr(i0,i1-i0+1);

          vec.push_back( boost::lexical_cast<T>( str ) );
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
   *  @param defaultValue Is stored initial default value of the property
   *  @param validator The validator to use for this property (this class will take ownership of the validator)
   *  @param direction Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
   */
  PropertyWithValue( const std::string &name, const TYPE& defaultValue, IValidator<TYPE> *validator = new NullValidator<TYPE>, const unsigned int direction = Direction::Input ) :
    Property( name, typeid( TYPE ), direction ),
    m_value( defaultValue ),
    m_initialValue( defaultValue ),
    m_validator( validator )
  {
  }

  /** Constructor
   *  @param name The name to assign to the property
   *  @param defaultValue Is stored initial default value of the property
   *  @param direction Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
   */
  PropertyWithValue( const std::string &name, const TYPE& defaultValue, const unsigned int direction) :
    Property( name, typeid( TYPE ), direction ),
    m_value( defaultValue ),
    m_initialValue( defaultValue ),
    m_validator( new NullValidator<TYPE> )
  {
  }
   
  /**Copy constructor
  *  Note the default value of the copied object is the initial value of original
  */
  PropertyWithValue( const PropertyWithValue& right ) :
    Property( right ),
    m_value( right.m_value ),
    m_initialValue( right.m_initialValue ),               //the default is the initial value of the original object
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

  /** Get the value the property was initialised with -its default value
   *  @return The default value
   */
  virtual std::string getDefault() const
  {
    PropertyUtility<TYPE> helper;
    return helper.value(m_initialValue);
  }

  /** Set the value of the property from a string representation.
   *  Note that "1" & "0" must be used for bool properties rather than true/false.
   *  @param value The value to assign to the property
   *  @return Returns "" if the assignment was successful or a user level description of the problem
   */
  virtual std::string setValue( const std::string& value )
  {
    try
    {
      PropertyUtility<TYPE> helper;
      TYPE result = m_value;
      helper.setValue(value, result);
      //Uses the assignment operator defined below which runs isValid() and throws based on the result
      *this = result;
      return "";
    }
    catch ( boost::bad_lexical_cast )
    {
      std::string error = "Could not set property " + name() +
        ". Can not convert \"" + value + "\" to " + type();
      g_log.debug() << error;
      return error;
    }
    catch ( std::invalid_argument& except)
    {
      g_log.debug() << except.what();
      return except.what();
    }
  }

  ///Copy assignment operator assigns only the value and the validator not the name, default (initial) value, etc.
  PropertyWithValue& operator=( const PropertyWithValue& right )
  {
    if ( &right == this ) return *this;
    // Chain the base class assignment operator for clarity (although it does nothing)
    Property::operator=( right );
    m_value = right.m_value;
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
    TYPE oldValue = m_value;
    m_value = value;
    std::string problem = this->isValid();
    if ( problem == "" )
    {
      return m_value;
    }
    else
    {
      m_value = oldValue;
      throw std::invalid_argument("Could not set property " + name() + ": " + problem);
    }
  }

  /** Allows you to get the value of the property via an expression like myProperty()
   *  @returns the value of the property
   */
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

  /** Check the value chosen for the property is OK, unless overidden it just calls the validator's isValid()
   *  N.B. Problems found in validator are written to the log
   *  if you override this function to do checking outside a validator may want to do more logging
   *  @returns "" if the value is valid or a discription of the problem
   */
  virtual std::string isValid() const
  {
	  return m_validator->isValid(m_value);
  }

  /** Indicates if the property's value is the same as it was when it was set
  *  N.B. Uses an unsafe comparison in the case of doubles, consider overriding if the value is a pointer or floating point type
  *  @return true if the value is the same as the initial value or false otherwise
  */
  virtual bool isDefault() const
  {
    return  m_initialValue == m_value;
  }

  /** Returns the type of the validator as a string
   *  @returns String describing the type of the validator
   */
  virtual const std::string getValidatorType() const
  {
    return m_validator->getType();
  }

  /**
   * Returns the validator
   * @returns A pointer to the IValidator interface
   */
  const IValidator<TYPE>* getValidator() const
  {
    return m_validator;
  }

  /** Returns the set of valid values for this property, if such a set exists.
   *  If not, it returns an empty vector.
   */
  virtual const std::vector<std::string> allowedValues() const
  {
    ListValidator *list = dynamic_cast<ListValidator*>(m_validator);

    FileValidator *file = dynamic_cast<FileValidator*>(m_validator);

    if (list)
    {
      const std::set<std::string>& vals = list->allowedValues();
      return std::vector<std::string>(vals.begin(), vals.end());
    }
    else if (file)
    {
	   return file->allowedValues();
    }
    else
    {
      // Just return an empty vector if the property does not have a ListValidator
      return std::vector<std::string>();
    }
  }

protected:
  /// The value of the property
  TYPE m_value;
  ///the property's default value which is also its initial value
  const TYPE m_initialValue;

private:

  /// Visitor validator class
  IValidator<TYPE> *m_validator;

  /// Static reference to the logger class
  static Logger& g_log;

  /// Private default constructor
  PropertyWithValue();
};

template <typename TYPE>
Logger& PropertyWithValue<TYPE>::g_log = Logger::get("PropertyWithValue");

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYWITHVALUE_H_*/
