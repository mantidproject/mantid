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
#include "MantidKernel/ClassMacros.h"

namespace Mantid { namespace Kernel
{
  namespace Units
  {
// Easily declare UnitLabels
#define DECLARE_UNIT_LABEL(ClassName) \
    class MANTID_KERNEL_DLL ClassName : public UnitLabel\
    {\
    public:\
      ClassName * clone() const;\
      const std::string ascii() const;\
      const std::wstring utf8() const;\
    };

    /// Empty label
    DECLARE_UNIT_LABEL(EmptyLabel);
    /// Second
    DECLARE_UNIT_LABEL(Second);
    /// Microsecond
    DECLARE_UNIT_LABEL(Microsecond);
    /// Nanosecond
    DECLARE_UNIT_LABEL(Nanosecond);
    /// Angstrom
    DECLARE_UNIT_LABEL(Angstrom);
    /// InverseAngstrom
    DECLARE_UNIT_LABEL(InverseAngstrom);
    /// InverseAngstromSq
    DECLARE_UNIT_LABEL(InverseAngstromSq);
    /// MilliElectronVolts
    DECLARE_UNIT_LABEL(MilliElectronVolts);
    /// Metre
    DECLARE_UNIT_LABEL(Metre);
    /// Nanometre
    DECLARE_UNIT_LABEL(Nanometre);
    /// Inverse centimeters
    DECLARE_UNIT_LABEL(InverseCM);


    //-------------------------------------------------------------------------

    /// Free text label with a different non-default constructor
    class MANTID_KERNEL_DLL TextLabel : public UnitLabel
    {
    public:
      /// Constructor
      TextLabel(const std::string & ascii, const std::wstring & utf8);
      /// Return a new object of this type
      TextLabel * clone() const;

      /// Return an ascii label for unit
      const std::string ascii() const;
      /// Return a utf-8 encoded label for unit
      const std::wstring utf8() const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(TextLabel);
      /// plain-text label
      std::string m_ascii;
      /// utf8 label
      std::wstring m_utf8;
    };

  // tidy up macro
  #undef DECLARE_UNIT_LABEL

  } // namespace Units
}} // namespace Mantid::Kernel

#endif  /* MANTID_KERNEL_UNITLABELTYPES_H_ */
