#ifndef MANTID_KERNEL_IVALIDATOR_H_
#define MANTID_KERNEL_IVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include <string>

namespace Mantid
{
namespace Kernel
{
/** IValidator is the basic interface for all validators for properties

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007
    
    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport IValidator
{
public:
  ///virtual Destructor
  virtual ~IValidator() {}

  /** Calls the validator
   *  
   *  @param value :: The value to be checked
   *  @returns An error message to display to users or an empty string on no error
   */
  std::string isValid(const TYPE &value) const
  {
    std::string failure = checkValidity(value);
    return failure;
  }

  /** The set of allowed values that this validator may have, if a discrete set exists.
   *  Overridden in applicable concrete validators; the base class just returns an empty set.
   *  @return The set of allowed values that this validator may have or an empty set
   */
  virtual std::set<std::string> allowedValues() const { return std::set<std::string>(); }
  
  /// Make a copy of the present type of validator
  virtual IValidator* clone() = 0;

protected:
  /** Checks the value based on the validator's rules
   * 
   *  @returns An error message to display to users or an empty string on no error
   */
  virtual std::string checkValidity(const TYPE &) const = 0;

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IVALIDATOR_H_*/
