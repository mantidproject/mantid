#ifndef MANTID_KERNEL_UNITLABEL_H_
#define MANTID_KERNEL_UNITLABEL_H_
/**
  Define a

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid
{
  namespace Kernel
  {
    /**
     * A base-class for the a class that is able to return
     * unit labels in different representations.
     */
    class MANTID_KERNEL_DLL UnitLabel
    {
    public:
      /// Virtual destructor
      virtual ~UnitLabel();
      /// "Virtual" copy constructor
      virtual UnitLabel * clone() const = 0;

      /// Equality operator with other label
      bool operator==(const UnitLabel & rhs) const;
      /// Equality operator with std::string
      bool operator==(const std::string & rhs) const;
      /// Equality operator with std::wtring
      bool operator==(const std::wstring & rhs) const;

      /// Return an ascii label for unit
      virtual const std::string ascii() const = 0;
      /// Return a utf-8 encoded label for unit
      virtual const std::wstring utf8() const = 0;

      /// Implicit conversion to std::string
      operator std::string() const;
    };

  } // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_UNITLABEL_H_ */
