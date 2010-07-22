#ifndef MANTID_KERNEL_REBINPARAMSVALIDATOR_H_
#define MANTID_KERNEL_REBINPARAMSVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IValidator.h"
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** Validator to check the format of a vector providing the rebin
    parameters to an algorithm.

    @author Russell Taylor, Tessella plc

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport RebinParamsValidator : public IValidator<std::vector<double> >
{
public:
  RebinParamsValidator() {}
  virtual ~RebinParamsValidator() {}
  Kernel::IValidator<std::vector<double> >* clone() { return new RebinParamsValidator(*this); }

private:
  std::string checkValidity( const std::vector<double> &value ) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_REBINPARAMSVALIDATOR_H_*/
