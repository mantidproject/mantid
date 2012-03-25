#ifndef MANTID_KERNEL_LISTVALIDATOR_H_
#define MANTID_KERNEL_LISTVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/TypedValidator.h"
#include <boost/lexical_cast.hpp>
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** ListValidator is a validator that requires the value of a property to be one of a defined list
    of possibilities. The default type is std::string

    @author Russell Taylor, Tessella Support Services plc
    @date 18/06/2008
 
    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
template<typename TYPE>
class ListValidator : public TypedValidator<TYPE>
{
public:
  /// Default constructor. Sets up an empty list of valid values.
  ListValidator() : TypedValidator<TYPE>()
  {}

  /** Constructor
   *  @param values :: A set of values consisting of the valid values     */
  explicit ListValidator(const std::set<TYPE>& values)
    : TypedValidator<TYPE>(), m_allowedValues(values.begin(), values.end())
  {
  }

  /** Constructor
   *  @param values :: A vector of the valid values     */
  explicit ListValidator(const std::vector<TYPE>& values):
    TypedValidator<TYPE>(), m_allowedValues(values.begin(), values.end())
  {
  }
  /// Destructor
  virtual ~ListValidator(){};
  /// Clone the validator
  IValidator_sptr clone() const{ return boost::make_shared<ListValidator<TYPE> >(*this); }
  /**
   * Returns the set of allowed values currently defined
   * @returns A set of allowed values that this validator will currently allow
   */
  std::set<std::string> allowedValues() const
  {
    /// The interface requires strings
    std::set<std::string> allowedStrings;
    auto cend = m_allowedValues.end();
    for(auto cit = m_allowedValues.begin(); cit != cend; ++cit)
    {
      allowedStrings.insert(boost::lexical_cast<std::string>(*cit));
    }
    return allowedStrings;
  }

  /**
   * Add value to the list of allowable values
   * @param value :: A value of the templated type
   */
  void addAllowedValue(const TYPE &value)
  {
    m_allowedValues.insert(value);
  }
private:
  /** Checks if the string passed is in the list
   *  @param value :: The value to test
   *  @return "" if the value is on the list, or "The value is not in the list of allowed values"
   */
  std::string checkValidity(const TYPE & value) const
  {
    if ( m_allowedValues.count(value) )
    {
      return "";
    }
    else
    {
      if ( isEmpty(value) ) return "Select a value";
      std::ostringstream os;
      os << "The value \"" << value << "\" is not in the list of allowed values";
      return os.str();
    }
  }

  /**
   * Is the value considered empty.
   * @param value :: The value to check
   * @return True if it is considered empty
   */
  template<typename T>
  bool isEmpty(const T & value) const{ UNUSED_ARG(value) return false; }
  /**
   * Is the value considered empty. Specialized string version to use empty
   * @param value :: The value to check
   * @return True if it is considered empty
   */
  bool isEmpty(const std::string & value) const { return value.empty(); }

  /// The set of valid values
  std::set<TYPE> m_allowedValues;
};

/// ListValidator<std::string> is used heavily
typedef ListValidator<std::string> StringListValidator;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LISTVALIDATOR_H_*/
