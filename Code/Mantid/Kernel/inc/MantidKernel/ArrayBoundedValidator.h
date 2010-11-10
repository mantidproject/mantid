#ifndef MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_
#define MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_

#include "MantidKernel/IValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** @class ArrayBoundedValidator ArrayBoundedValidator.h Kernel/ArrayBoundedValidator.h

    ArrayBoundedValidator is a validator that requires all values in an array
    to be between upper or lower bounds, or both.

    @author Michael Reuter, NScD Oak Ridge National Laboratory
    @date 09/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ArrayBoundedValidator: public IValidator<std::vector<TYPE> >
{
public:
  ArrayBoundedValidator();
  ArrayBoundedValidator(const ArrayBoundedValidator<TYPE> & abv);
  ArrayBoundedValidator(const TYPE lowerBound, const TYPE upperBound);
  ArrayBoundedValidator(BoundedValidator<TYPE> &bv);
  virtual ~ArrayBoundedValidator();

  IValidator<std::vector<TYPE> >* clone();
  BoundedValidator<TYPE>* getValidator() const;
  std::string isValid(const std::vector<TYPE> &value) const;

private:
  std::string checkValidity( const std::vector<TYPE> &value ) const;

  /// The object used to do the actual validation
  BoundedValidator<TYPE> *boundVal;
};

} // Kernel
} // Mantid

#endif /* MANTID_KERNEL_ARRAYBOUNDEDVALIDATOR_H_ */
