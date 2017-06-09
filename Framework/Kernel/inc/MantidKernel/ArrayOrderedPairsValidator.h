#ifndef MANTID_KERNEL_ARRAYORDEREDPAIRSVALIDATOR_H_
#define MANTID_KERNEL_ARRAYORDEREDPAIRSVALIDATOR_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/TypedValidator.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** @class ArrayOrderedPairsValidator ArrayOrderedPairsValidator.h
   Kernel/ArrayOrderedPairsValidator.h

    ArrayOrderedPairsValidator validates that an array contains a sequence of
    ordered pairs of numbers.

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <typename TYPE>
class MANTID_KERNEL_DLL ArrayOrderedPairsValidator
    : public TypedValidator<std::vector<TYPE>> {
public:
  /// Clone the current state
  IValidator_sptr clone() const override;

private:
  std::string checkValidity(const std::vector<TYPE> &value) const override;
};

} // Kernel
} // Mantid

#endif /* MANTID_KERNEL_ARRAYORDEREDPAIRSVALIDATOR_H_ */
