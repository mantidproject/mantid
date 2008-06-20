#ifndef MANTID_KERNEL_LISTVALIDATOR_H_
#define MANTID_KERNEL_LISTVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IValidator.h"
#include <set>
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** ListValidator is a validator that requires the value of a property to be one of a defined list
    of possibilities. At present, this validator is only available for properties of type std::string

    @author Russell Taylor, Tessella Support Services plc
    @date 18/06/2008
 
    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport ListValidator : public IValidator<std::string>
{
public:
  /// Default constructor. Sets up an empty list of valid values.
  ListValidator() : IValidator<std::string>(), m_allowedValues()
  {}
	
  /** Constructor
   *  @param values A vector of strings containing the valid values 
   */
  explicit ListValidator(const std::vector<std::string> values) : 
    IValidator<std::string>(),
    m_allowedValues(values.begin(),values.end())
  {}
  
	/// Destructor
	virtual ~ListValidator() {}
	
  /** Checks whether the value provided is on the list of 'allowed' values
   *  @param value The value to test
   *  @return True if the value is on the list, false otherwise
   */
  const bool isValid(const std::string &value) const
  {
    if ( m_allowedValues.count(value) )
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  /// Returns the set of valid values
  const std::set<std::string>& allowedValues() const
  {
    return m_allowedValues;
  }
  
  /// Adds the argument to the set of valid values
  void addAllowedValue(const std::string &value)
  {
    m_allowedValues.insert(value);
  }
  
  IValidator<std::string>* clone() { return new ListValidator(*this); }
  
private:
  /// The set of valid values
  std::set<std::string> m_allowedValues;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LISTVALIDATOR_H_*/
