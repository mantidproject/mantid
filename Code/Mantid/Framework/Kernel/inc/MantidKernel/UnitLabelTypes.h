#ifndef MANTID_KERNEL_UNITLABELTYPES_H_
#define MANTID_KERNEL_UNITLABELTYPES_H_
/**

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
#include "MantidKernel/UnitLabel.h"

namespace Mantid { namespace Kernel
{
  namespace Units
  {
    /// Empty label
    class MANTID_KERNEL_DLL EmptyLabel : public UnitLabel
    {
    public:
      /// Return an ascii label for unit
      const std::string ascii() const;
      /// Return a utf-8 encoded label for unit
      const std::wstring utf8() const;
    };

    /// Concrete label type for microseconds
    class MANTID_KERNEL_DLL Microseconds : public UnitLabel
    {
    public:
      /// Return an ascii label for unit
      const std::string ascii() const;
      /// Return a utf-8 encoded label for unit
      const std::wstring utf8() const;
    };
  } // namespace Units
}} // namespace Mantid::Kernel

#endif  /* MANTID_KERNEL_UNITLABELTYPES_H_ */
