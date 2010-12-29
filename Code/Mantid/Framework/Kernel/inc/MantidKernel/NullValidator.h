#ifndef MANTID_KERNEL_NULLVALIDATOR_H_
#define MANTID_KERNEL_NULLVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IValidator.h"

namespace Mantid
{
namespace Kernel
{
/** @class NullValidator NullValidator.h Kernel/NullValidator.h

    NullValidator is a validator that doesn't.

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
class DLLExport NullValidator : public IValidator<TYPE>
{
public:
  IValidator<TYPE>* clone() { return new NullValidator(*this); }

private:
  /** Always returns valid, that is ""
   *  @returns an empty string
   */
   std::string checkValidity( const TYPE &) const { return ""; }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_NULLVALIDATOR_H_*/
