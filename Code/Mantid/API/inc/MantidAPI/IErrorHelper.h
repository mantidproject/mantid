#ifndef MANTID_KERNEL_IERRORHELPER_H_
#define MANTID_KERNEL_IERRORHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/TripleRef.h"

namespace Mantid
{
namespace API
{

/** @class IErrorHelper IErrorHelper.h Kernel/IErrorHelper.h

    An interface class for helper classes for calculating operations on values with assi=ociated errors

    @author Nick Draper, Tessella Support Services plc
    @date 24/01/2008
    
    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
class DLLExport IErrorHelper
{
public:
  ///Typedef for the value type of the ErrorHelper
  typedef TripleRef<double> value_type;
  
  /**Addition
  * @param lhs The lhs value
  * @param rhs The rhs value
  * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
  */
  virtual const value_type plus (const value_type& lhs,const value_type& rhs) const=0;
  
  /**Subtraction
  * @param lhs The lhs value
  * @param rhs The rhs value
  * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
  */
  virtual const value_type minus (const value_type& lhs,const value_type& rhs) const=0;
  
  /**Multiplication
  * @param lhs The lhs value
  * @param rhs The rhs value
  * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
  */
  virtual const value_type multiply (const value_type& lhs,const value_type& rhs) const=0;
  
  /**Division
  * @param lhs The lhs value
  * @param rhs The rhs value
  * @returns The result with value and error caluculated, all other values wil be passed through from the lhs value
  */
  virtual const value_type divide (const value_type& lhs,const value_type& rhs) const=0;

  ///Virtual Destructor
  virtual ~IErrorHelper() {}
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IERRORHELPER_H_*/
