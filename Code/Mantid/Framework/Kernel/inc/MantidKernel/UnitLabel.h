#ifndef MANTID_KERNEL_UNITLABEL_H_
#define MANTID_KERNEL_UNITLABEL_H_
/**
  Define a

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include "MantidKernel/ClassMacros.h"
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
      /// Type that contains a plain-text string
      typedef std::string AsciiString;
      /// Type that can hold a unicode string. This may vary per-platform depending on the
      /// width of the the built-in std::wstring
      typedef std::wstring Utf8String;

      /// Constructor giving labels as ascii, unicode, and latex respectively
      UnitLabel(const AsciiString & ascii, const Utf8String & unicode, const AsciiString & latex);
      /// Constructor giving both labels as ascii
      UnitLabel(const AsciiString & ascii);
      /// Constructor giving both labels as ascii using a C-style string
      UnitLabel(const char * ascii);

      /// Equality operator with other label
      bool operator==(const UnitLabel & rhs) const;
      /// Equality operator with std::string
      bool operator==(const std::string & rhs) const;
      /// Equality operator with c-style string
      bool operator==(const char * rhs) const;
      /// Equality operator with std::wstring
      bool operator==(const std::wstring & rhs) const;

      /// Inqquality operator with other label
      bool operator!=(const UnitLabel & rhs) const;
      /// Inequality operator with std::string
      bool operator!=(const std::string & rhs) const;
      /// Inequality operator with c-style string
      bool operator!=(const char * rhs) const;
      /// Inequality operator with std::wstring
      bool operator!=(const std::wstring & rhs) const;

      /// Return an ascii label for unit
      const AsciiString & ascii() const;
      /// Return a utf-8 encoded label for unit
      const Utf8String &utf8() const;
      /// Return an ascii latex compatible label for unit
      const AsciiString & latex() const;

      /// Implicit conversion to std::string
      operator std::string() const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(UnitLabel);

      /// Value of plain-text label
      std::string m_ascii;
      /// Value of utf-8 encoded string
      std::wstring m_utf8;
      /// Value of latex label
      std::string m_latex;
    };

  } // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_UNITLABEL_H_ */
