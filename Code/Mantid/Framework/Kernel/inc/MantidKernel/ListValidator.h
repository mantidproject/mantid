#ifndef MANTID_KERNEL_LISTVALIDATOR_H_
#define MANTID_KERNEL_LISTVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ListAnyValidator.h"

namespace Mantid
{
namespace Kernel
{
/** ListValidator is a validator that requires the value of a property to be one of a defined list
    of possibilities. At present, this validator is only available for properties of type std::string

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

class  ListValidator : public ListAnyValidator<std::string>
{
public:
  /// Default constructor. Sets up an empty list of valid values.
  ListValidator():ListAnyValidator<std::string>()
  {}

  /** Constructor
   *  @param values :: A set of values consisting of the valid values     */
  explicit ListValidator(const std::set<std::string>& values):
        ListAnyValidator<std::string>(values)
  {}

  /** Constructor
   *  @param values :: A vector of the valid values     */
  explicit ListValidator(const std::vector<std::string>& values):
        ListAnyValidator<std::string>(values)
  {}

  // overload this function to keep python happy (temporary?)
  void addAllowedValue(const std::string &value)
  {
    ListAnyValidator<std::string>::addAllowedValue(value);
  }

  virtual ~ListValidator(){};

  virtual IValidator<std::string>* clone(){ return new ListValidator(*this); }

  
protected:
  /** Checks if the string passed is in the list
   *  @param value :: The value to test
   *  @return "" if the value is on the list, or "The value is not in the list of allowed values"
   */
  std::string checkValidity(const std::string &value) const
  {
    if ( m_allowedValues.count(value) )
    {
      return "";
    }
    else
    {
      if ( value.empty() ) return "Select a value";
      else return "The value \"" + value + "\" is not in the list of allowed values";
    }
  }

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LISTVALIDATOR_H_*/
