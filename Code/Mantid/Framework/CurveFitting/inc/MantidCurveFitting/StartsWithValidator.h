#ifndef MANTID_KERNEL_STARTSWITHVALIDATOR_H_
#define MANTID_KERNEL_STARTSWITHVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ListValidator.h"
#include <boost/lexical_cast.hpp>
#include <vector>

namespace Mantid
{
namespace CurveFitting
{
/** StartsWithValidator is a validator that requires the value of a property to start with of one 
    of the strings in a defined list of possibilities.

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
class StartsWithValidator : public Kernel::StringListValidator
{
public:
  /**
   * Constructor.
   * @param values :: A vector with the allowed values.
   */
  StartsWithValidator(const std::vector<std::string>& values):Kernel::StringListValidator(values){}
  /**
   * Constructor.
   * @param values :: A set with the allowed values.
   */
  StartsWithValidator(const std::set<std::string>& values):Kernel::StringListValidator(values){}
protected:
  /** Checks if the string passed is in the list
   *  @param value :: The value to test
   *  @return "" if the value is on the list, or "The value does not start with any of the allowed values"
   */
  std::string checkValidity(const std::string & value) const
  {
    for(auto it = m_allowedValues.begin(); it != m_allowedValues.end(); ++it )
    {
      if ( value.substr(0, it->size()) == *it )
      {
        return "";
      }
    }
    if ( isEmpty(value) ) return "Select a value";
    std::ostringstream os;
    os << "The value \"" << value << "\" does not start with any of the allowed values";
    return os.str();
  }

};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_KERNEL_STARTSWITHVALIDATOR_H_*/
