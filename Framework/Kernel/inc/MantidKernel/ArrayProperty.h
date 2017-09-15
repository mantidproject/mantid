#ifndef MANTID_KERNEL_ARRAYPROPERTY_H_
#define MANTID_KERNEL_ARRAYPROPERTY_H_

#include "DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/Property.h"
#include "PropertyWithValue.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** Support for a property that holds an array of values.
    Implemented as a PropertyWithValue that holds a vector of the desired type.
    This class is really a convenience class to aid in the declaration of the
    property - there's no problem directly using a PropertyWithValue of vector
   type.

    @author Russell Taylor, Tessella Support Services plc
    @date 27/02/2008

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
template <typename T>
class DLLExport ArrayProperty : public PropertyWithValue<std::vector<T>> {
public:
  ArrayProperty(const std::string &name, const std::vector<T> &vec,
                IValidator_sptr validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, IValidator_sptr validator,
                const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name,
                const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, const std::string &values,
                IValidator_sptr validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input);

  ArrayProperty<T> *clone() const override;

  // Unhide the base class assignment operator
  using PropertyWithValue<std::vector<T>>::operator=;

  std::string value() const override;

  std::string setValue(const std::string &value) override;
  // May want to add specialisation the the class later, e.g. setting just one
  // element of the vector

private:
  // This method is a workaround for the C4661 compiler warning in visual
  // studio. This allows the template declaration and definition to be separated
  // in different files. See stack overflow article for a more detailed
  // explanation:
  // https://stackoverflow.com/questions/44160467/warning-c4661no-suitable-definition-provided-for-explicit-template-instantiatio
  // https://stackoverflow.com/questions/33517902/how-to-export-a-class-derived-from-an-explicitly-instantiated-template-in-visual
  void visualStudioC4661Workaround();
};

template <>
MANTID_KERNEL_DLL void ArrayProperty<int>::visualStudioC4661Workaround();

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ARRAYPROPERTY_H_*/
