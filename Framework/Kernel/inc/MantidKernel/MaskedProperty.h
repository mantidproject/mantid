#ifndef MANTID_KERNEL_MASKEDPROPERTY_H_
#define MANTID_KERNEL_MASKEDPROPERTY_H_

#include "MantidKernel/PropertyWithValue.h"
#include <string>

/** A property class for masking the properties. It inherits from
   PropertyWithValue.
     This class masks the properties and useful when the property value is not
   to be
         displayed  in the user interface widgets like algorithm history
   display,
         log window etc and log file .The masked property will be displayed.
     This class is specialised for string and masks the property value with
   charcater "*"

    @class Mantid::Kernel::MaskedProperty

    Copyright &copy;  2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

namespace Mantid {
namespace Kernel {
template <typename TYPE = std::string>
class MANTID_KERNEL_DLL MaskedProperty
    : public Kernel::PropertyWithValue<TYPE> {
public:
  /// Constructor with a validator
  MaskedProperty(const std::string &name, TYPE defaultvalue,
                 IValidator_sptr validator = IValidator_sptr(new NullValidator),
                 const unsigned int direction = Direction::Input);
  /// Constructor with a validator without validation
  MaskedProperty(const std::string &name, const TYPE &defaultvalue,
                 const unsigned int direction);
  /// "virtual" copy constructor
  MaskedProperty *clone() const;

  /// Mask out the out the value in the history
  virtual const Kernel::PropertyHistory createHistory() const;

  /** This method returns the masked property value
   */
  TYPE getMaskedValue() const;

  // Unhide the PropertyWithValue assignment operator
  using Kernel::PropertyWithValue<TYPE>::operator=;

private:
  /// Perform the actual masking
  void doMasking() const;

  mutable TYPE m_maskedValue; ///< the masked value
};
}
}
#endif
