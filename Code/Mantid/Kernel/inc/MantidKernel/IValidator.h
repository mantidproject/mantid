#ifndef MANTID_KERNEL_IVALIDATOR_H_
#define MANTID_KERNEL_IVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{
/** @class IValidator IValidator.h Kernel/IValidator.h

    IValidator is the basic interface for all validators for properties

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007
    
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
class DLLExport IValidator
{
public:
  ///virtual Destructor
  virtual ~IValidator() {}

  /** Checks the value based on the validators rules
   * 
   *  @param value The value to test
   */
  virtual const bool isValid(const TYPE &value) const = 0;

  /// Make a copy of the present type of validator
  virtual IValidator* clone() = 0;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IVALIDATOR_H_*/
