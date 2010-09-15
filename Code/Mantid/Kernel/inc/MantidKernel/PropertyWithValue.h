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
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <Poco/StringTokenizer.h>
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** The concrete, templated class for properties.
    The supported types at present are int, double, bool & std::string.

    With reference to the Gaudi structure, this class can be seen as the equivalent of both the
    Gaudi class of the same name and its sub-classses.

    @class Mantid::Kernel::PropertyWithValue

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 14/11/2007

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

// --------------------- convert values to strings

/// Convert values to strings.
template <typename T>
std::string toString(const T& value)
{
  return boost::lexical_cast<std::string>(value);
}

/// Throw an exception if a shared pointer is converted to a string.
template <typename T>
std::string toString(const boost::shared_ptr<T>& value)
{
  (void) value; //avoid compiler warning
  throw boost::bad_lexical_cast();
}

/// Specialisation for a property of type std::vector.
template <typename T>
std::string toString(const std::vector<T>& value)
{
  std::stringstream result;
  std::size_t vsize = value.size();
  for (std::size_t i = 0; i < vsize; ++i)
  {
    result << value[i];
    if (i + 1 != vsize)
      result << ",";
  }
  return result.str();
}

// ------------- Convert strings to values
template <typename T>
inline void appendValue(const std::string& strvalue, std::vector<T>& value)
{
  // try to split the string
  std::size_t pos = strvalue.find(':');
  if (pos == std::string::npos)
  {
    pos = strvalue.find('-', 1);
  }

  // just convert the whole thing into a value
  if (pos == std::string::npos){
      value.push_back(boost::lexical_cast<T>(strvalue));
      return;
  }

  // convert the input string into boundaries and run through a list
  T start = boost::lexical_cast<T>(strvalue.substr(0, pos));
  T stop  = boost::lexical_cast<T>(strvalue.substr(pos + 1));
  for (T i = start; i <= stop; i++)
    value.push_back(i);
}

template <typename T>
void toValue(const std::string& strvalue, T& value)
{
  value = boost::lexical_cast<T>( strvalue );
}

template <typename T>
void toValue(const std::string&, boost::shared_ptr<T>&)
{
  throw boost::bad_lexical_cast();
}

template <typename T>
void toValue(const std::string& strvalue, std::vector<T>& value)
{
  // Split up comma-separated properties
  typedef Poco::StringTokenizer tokenizer;
  tokenizer values(strvalue, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  value.clear();
  value.reserve(values.count());

  for (tokenizer::Iterator it = values.begin(); it != values.end(); ++it)
  {
    value.push_back(boost::lexical_cast<T>(*it));
  }
}

template <>
inline void toValue<int>(const std::string& strvalue, std::vector<int>& value)
{
  // Split up comma-separated properties
  typedef Poco::StringTokenizer tokenizer;
  tokenizer values(strvalue, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  value.clear();
  value.reserve(values.count());

  for (tokenizer::Iterator it = values.begin(); it != values.end(); ++it)
  {
    appendValue(*it, value);
  }
}

template <>
inline void toValue<unsigned int>(const std::string& strvalue, std::vector<unsigned int>& value)
{
  // Split up comma-separated properties
  typedef Poco::StringTokenizer tokenizer;
  tokenizer values(strvalue, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  value.clear();
  value.reserve(values.count());

  for (tokenizer::Iterator it = values.begin(); it != values.end(); ++it)
  {
    appendValue(*it, value);
  }
}

//------------------------------------------------------------------------------------------------
// Templated += operator functions for specific types
template<typename T>
inline void addingOperator(T& lhs, const T& rhs)
{
  lhs += rhs;
}

template<typename T>
inline void addingOperator(std::vector<T>& lhs, const std::vector<T>& rhs)
{
  //This concatenates the two
  lhs.insert(lhs.end(), rhs.begin(), rhs.end());
}

template<typename T>
inline void addingOperator(boost::shared_ptr<T>& lhs, const boost::shared_ptr<T>& rhs)
{
  (void) lhs; //avoid compiler warning
  (void) rhs; //avoid compiler warning
  throw Exception::NotImplementedError("PropertyWithValue.h: += operator not implemented for boost::shared_ptr");
}







//------------------------------------------------------------------------------------------------
// Now the PropertyWithValue class itself
//------------------------------------------------------------------------------------------------

template <typename TYPE>
class PropertyWithValue : public Property
{
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
  /// 'Virtual copy constructor'
  Property* clone() { return new PropertyWithValue<TYPE>(*this); }

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
    return toString(m_value);
  }

  /** Get the value the property was initialised with -its default value
   *  @return The default value
   */
  virtual std::string getDefault() const
  {
    return toString(m_initialValue);
  }

  /** Set the value of the property from a string representation.
   *  Note that "1" & "0" must be used for bool properties rather than true/false.
   *  @param value The value to assign to the property
   *  @return Returns "" if the assignment was successful or a user level description of the problem
   */
  virtual std::string setValue(const std::string& value )
  {
    try
    {
      TYPE result = m_value;
      toValue(value, result);
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
      g_log.debug() << "Could not set property " << name() << ": " << except.what();
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
    // Delete the existing validator
    delete m_validator;
    m_validator = right.m_validator->clone();
    return *this;
  }


  //--------------------------------------------------------------------------------------
  ///Add the value of another property
  virtual PropertyWithValue& operator+=( Property * right )
  {
    PropertyWithValue * rhs = dynamic_cast< PropertyWithValue * >(right);

    //This function basically does:
    //  m_value += rhs->m_value; for values
    //  or concatenates vectors for vectors
    addingOperator(m_value, rhs->m_value);

    return *this;
  }



  //--------------------------------------------------------------------------------------
  /** Assignment operator.
   *  Allows assignment of a new value to the property by writing,
   *  e.g., myProperty = 3;
   *  @param value The new value to assign to the property
   *  @return the reference to itself
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
      throw std::invalid_argument(problem);
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
   * @return the value
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

  /** Returns the set of valid values for this property, if such a set exists.
   *  If not, it returns an empty vector.
   *  @return Returns the set of valid values for this property, or it returns an empty vector.
   */
  virtual std::set<std::string> allowedValues() const
  {
    return m_validator->allowedValues();
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


//
////==============================================================================================
//// Specialization of the Template
//template <typename TYPE>
//class PropertyWithValue< std::vector<TYPE> >
//{
//PropertyWithValue& operator+=( const PropertyWithValue& right )
//{
//  //m_value.push_back(12);
//  //m_value += right.m_value;
//  return *this;
//}
//};



template <typename TYPE>
Logger& PropertyWithValue<TYPE>::g_log = Logger::get("PropertyWithValue");

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYWITHVALUE_H_*/
