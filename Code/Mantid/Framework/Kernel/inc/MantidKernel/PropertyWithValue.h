#ifndef MANTID_KERNEL_PROPERTYWITHVALUE_H_
#define MANTID_KERNEL_PROPERTYWITHVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/NullValidator.h"

#ifndef Q_MOC_RUN
# include <boost/lexical_cast.hpp>
# include <boost/shared_ptr.hpp>
#endif

#include <Poco/StringTokenizer.h>
#include <vector>
#include "MantidKernel/IPropertySettings.h"


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

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// --------------------- convert values to strings
namespace {
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
  UNUSED_ARG(value);
  throw boost::bad_lexical_cast();
}

/// Specialisation for a property of type std::vector.
template <typename T>
std::string toString(const std::vector<T>& value, const std::string & delimiter = ",")
{
  std::stringstream result;
  std::size_t vsize = value.size();
  for (std::size_t i = 0; i < vsize; ++i)
  {
    result << value[i];
    if (i + 1 != vsize)
      result << delimiter;
  }
  return result.str();
}

/// Specialisation for a property of type std::vector<std::vector>.
template <typename T>
std::string toString(const std::vector<std::vector<T> >& value, const std::string & outerDelimiter = ",", 
                     const std::string & innerDelimiter = "+")
{
  std::stringstream result;
  std::size_t vsize = value.size();
  for (std::size_t i = 0; i < vsize; ++i)
  {
    std::size_t innervsize = value[i].size();
    for (std::size_t j = 0; j < innervsize; ++j)
    {
      result << value[i][j];
      if (j + 1 != innervsize)
        result << innerDelimiter;
    }

    if (i + 1 != vsize)
      result << outerDelimiter;
  }
  return result.str();
}

/// Specialisation for any type, should be appropriate for properties with a single value.
template <typename T>
int findSize(const T&)
{
  return 1;
}

/// Specialisation for properties that are of type vector.
template <typename T>
int findSize(const std::vector<T>& value)
{
  return static_cast<int>(value.size());
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

template <typename T>
void toValue(const std::string& strvalue, std::vector<std::vector<T> >& value, const std::string & outerDelimiter = ",", 
                     const std::string & innerDelimiter = "+")
{
  typedef Poco::StringTokenizer tokenizer;
  tokenizer tokens(strvalue, outerDelimiter, tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);

  value.clear();
  value.reserve(tokens.count());

  for (tokenizer::Iterator oIt = tokens.begin(); oIt != tokens.end(); ++oIt)
  {
    tokenizer values(*oIt, innerDelimiter, tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
    std::vector<T> vect;

    for (tokenizer::Iterator iIt = values.begin(); iIt != values.end(); ++iIt)
      vect.push_back(boost::lexical_cast<T>(*iIt));

    value.push_back(vect);
  }
}

/// Macro for the vector<int> specializations
#define PROPERTYWITHVALUE_TOVALUE(type) \
  template <> \
  inline void toValue<type>(const std::string& strvalue, std::vector<type>& value) \
  { \
    typedef Poco::StringTokenizer tokenizer; \
    tokenizer values(strvalue, ",", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM); \
    value.clear(); \
    value.reserve(values.count()); \
    for (tokenizer::Iterator it = values.begin(); it != values.end(); ++it) \
    { \
      appendValue(*it, value); \
    } \
  }
  
  PROPERTYWITHVALUE_TOVALUE(int);
  PROPERTYWITHVALUE_TOVALUE(long);
  PROPERTYWITHVALUE_TOVALUE(uint32_t);
  PROPERTYWITHVALUE_TOVALUE(uint64_t);
  #if defined(__APPLE__)
    PROPERTYWITHVALUE_TOVALUE(unsigned long);
  #endif

// Clear up the namespace
#undef PROPERTYWITHVALUE_TOVALUE

//------------------------------------------------------------------------------------------------
// Templated += operator functions for specific types
template<typename T>
inline void addingOperator(T& lhs, const T& rhs)
{
  // The cast here (and the expansion of the compound operator which that
  // necessitates) is because if this function is created for a template
  // type narrower than an int, the compiler will expande the operands to
  // ints which leads to a compiler warning when it's assigned back to the
  // narrower type.
  lhs = static_cast<T>(lhs + rhs);
}

template<typename T>
inline void addingOperator(std::vector<T>& lhs, const std::vector<T>& rhs)
{
  //This concatenates the two
  if ( &lhs != &rhs)
  {
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
  }
  else
  {
    std::vector<T> rhs_copy(rhs);
    lhs.insert(lhs.end(), rhs_copy.begin(), rhs_copy.end());
  }
}

template<>
inline void addingOperator(bool&, const bool&)
{
  throw Exception::NotImplementedError("PropertyWithValue.h: += operator not implemented for type bool");
}

template<typename T>
inline void addingOperator(boost::shared_ptr<T>& lhs, const boost::shared_ptr<T>& rhs)
{
  UNUSED_ARG(lhs);
  UNUSED_ARG(rhs);
  throw Exception::NotImplementedError("PropertyWithValue.h: += operator not implemented for boost::shared_ptr");
}
}
//------------------------------------------------------------------------------------------------
// Now the PropertyWithValue class itself
//------------------------------------------------------------------------------------------------

template <typename TYPE>
class DLLExport PropertyWithValue : public Property
{
public:
  /** Constructor
   *  @param name :: The name to assign to the property
   *  @param defaultValue :: Is stored initial default value of the property
   *  @param validator :: The validator to use for this property
   *  @param direction :: Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
   */
  PropertyWithValue( const std::string &name, const TYPE& defaultValue, 
                     IValidator_sptr validator = IValidator_sptr(new NullValidator),
                     const unsigned int direction = Direction::Input) :
    Property( name, typeid( TYPE ), direction ),
    m_value( defaultValue ),
    m_initialValue( defaultValue ),
    m_validator( validator )
  {
  }

  /** Constructor
   *  @param name :: The name to assign to the property
   *  @param defaultValue :: Is stored initial default value of the property
   *  @param direction :: Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) property
   */
  PropertyWithValue( const std::string &name, const TYPE& defaultValue, const unsigned int direction) :
    Property( name, typeid( TYPE ), direction ),
    m_value( defaultValue ),
    m_initialValue( defaultValue ),
    m_validator( boost::make_shared<NullValidator>())
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
  PropertyWithValue<TYPE>* clone() const { return new PropertyWithValue<TYPE>(*this); }

  /// Virtual destructor
  virtual ~PropertyWithValue()
  {
  }

  /** Get the value of the property as a string
   *  @return The property's value
   */
  virtual std::string value() const
  {
    return toString(m_value);
  }

  /**
   * Deep comparison.
   * @param rhs The other property to compare to.
   * @return true if the are equal.
   */
  virtual bool operator==(const PropertyWithValue<TYPE> & rhs) const
  {
    if (this->name() != rhs.name())
      return false;
    return (m_value == rhs.m_value);
  }

  /**
   * Deep comparison (not equal).
   * @param rhs The other property to compare to.
   * @return true if the are not equal.
   */
  virtual bool operator!=(const PropertyWithValue<TYPE> & rhs) const
  {
    return !(*this == rhs);
  }

  /** Get the size of the property.
  */
  virtual int size() const
  {
    return findSize(m_value);
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
   *  @param value :: The value to assign to the property
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
    catch ( boost::bad_lexical_cast&)
    {
      std::string error = "Could not set property " + name() +
        ". Can not convert \"" + value + "\" to " + type();
      g_logger.debug() << error;
      return error;
    }
    catch ( std::invalid_argument& except)
    {
      g_logger.debug() << "Could not set property " << name() << ": " << except.what();
      return except.what();
    }
    return "";
  }

  /**
   * Set a property value via a DataItem
   * @param data :: A shared pointer to a data item
   * @return "" if the assignment was successful or a user level description of the problem
   */
  virtual std::string setDataItem(const boost::shared_ptr<DataItem> data)
  {
    // Pass of the helper function that is able to distinguish whether
    // the TYPE of the PropertyWithValue can be converted to a shared_ptr<DataItem>
    return setTypedValue(data, boost::is_convertible<TYPE, boost::shared_ptr<DataItem> >());
  }


  ///Copy assignment operator assigns only the value and the validator not the name, default (initial) value, etc.
  PropertyWithValue& operator=( const PropertyWithValue& right )
  {
    if ( &right == this ) return *this;
    m_value = right.m_value;
    m_validator = right.m_validator->clone();
    return *this;
  }

  //--------------------------------------------------------------------------------------
  /** Add the value of another property
  * @param right the property to add
  * @return the sum
  */
  virtual PropertyWithValue& operator+=( Property const * right )
  {
    PropertyWithValue const * rhs = dynamic_cast< PropertyWithValue const * >(right);

    if (rhs)
    {
      //This function basically does:
      //  m_value += rhs->m_value; for values
      //  or concatenates vectors for vectors
      addingOperator(m_value, rhs->m_value);
    }
    else
      g_logger.warning() << "PropertyWithValue " << this->name()
                       << " could not be added to another property of the same name but incompatible type.\n";

    return *this;
  }



  //--------------------------------------------------------------------------------------
  /** Assignment operator.
   *  Allows assignment of a new value to the property by writing,
   *  e.g., myProperty = 3;
   *  @param value :: The new value to assign to the property
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
    else if ( problem == "_alias" )
    {
      m_value = getValueForAlias( value );
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
  virtual std::vector<std::string> allowedValues() const
  {
    return m_validator->allowedValues();
  }

  /**
   * Replace the current validator with the given one
   * @param newValidator :: A replacement validator
   */
  virtual void replaceValidator(IValidator_sptr newValidator)
  {
    m_validator = newValidator;
  }

protected:
  /// The value of the property
  TYPE m_value;
  ///the property's default value which is also its initial value
  const TYPE m_initialValue;

private:
  /**
   * Set the value of the property via a reference to another property.  
   * If the value is unacceptable the value is not changed but a string is returned.
   * The value is only accepted if the other property has the same type as this
   * @param right :: A reference to a property.
   */
  virtual std::string setValueFromProperty( const Property& right )
  {
    auto prop = dynamic_cast<const PropertyWithValue<TYPE>*>(&right);
    if ( !prop )
    {
      return "Could not set value: properties have different type.";
    }
    m_value = prop->m_value;
    return "";
  }

  /**
   * Helper function for setValue(DataItem_sptr). Uses boost type traits to ensure
   * it is only used if U is a type that is convertible to boost::shared_ptr<DataItem>
   * @param value :: A object of type convertible to boost::shared_ptr<DataItem>
   */
  template<typename U>
  std::string setTypedValue(const U & value, const boost::true_type &)
  {
    TYPE data = boost::dynamic_pointer_cast<typename TYPE::element_type>(value);
    std::string msg;
    if ( data )
    {
      try
      {
        (*this) = data;
      }
      catch(std::invalid_argument& exc)
      {
        msg = exc.what();
      }
    }
    else
    {
      msg = "Invalid DataItem. The object type (" + std::string(typeid(value).name()) 
        + ") does not match the declared type of the property (" + std::string(this->type()) + ").";
    }
        return msg;
    }

  /**
   * Helper function for setValue(DataItem_sptr). Uses boost type traits to ensure
   * it is only used if U is NOT a type that is convertible to boost::shared_ptr<DataItem>
   * @param value :: A object of type convertible to boost::shared_ptr<DataItem>
   */
    template<typename U>
    std::string setTypedValue(const U & value, const boost::false_type &)
    {
      UNUSED_ARG(value);
      return "Attempt to assign object of type DataItem to property (" + name() + ") of incorrect type";
    }

  /** Return value for a given alias.
   * @param alias :: An alias for a value. If a value cannot be found throw an invalid_argument exception.
   * @return :: A value.
   */
  const TYPE getValueForAlias(const TYPE& alias) const
  {
    std::string strAlias = toString( alias );
    std::string strValue = m_validator->getValueForAlias( strAlias );
    TYPE value;
    toValue( strValue, value );
    return value;
  }

  /// Visitor validator class
  IValidator_sptr m_validator;

  /// Static reference to the logger class
  static Logger g_logger;

  /// Private default constructor
  PropertyWithValue();
};


template <typename TYPE>
Logger PropertyWithValue<TYPE>::g_logger("PropertyWithValue");

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYWITHVALUE_H_*/
