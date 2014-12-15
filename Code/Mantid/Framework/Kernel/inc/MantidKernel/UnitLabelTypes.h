#ifndef MANTID_KERNEL_UNITLABELTYPES_H_
#define MANTID_KERNEL_UNITLABELTYPES_H_
/**

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
#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/ClassMacros.h"

namespace Mantid { namespace Kernel
{
  namespace Units
  {
    /**
     * A simple class containing common symbol types
     */
    class MANTID_KERNEL_DLL Symbol
    {
    public:
      /// Empty label
      static const UnitLabel EmptyLabel;
      /// Second
      static const UnitLabel Second;
      /// Microsecond
      static const UnitLabel Microsecond;
      /// Nanosecond
      static const UnitLabel Nanosecond;
      /// Angstrom
      static const UnitLabel Angstrom;
      /// InverseAngstrom
      static const UnitLabel InverseAngstrom;
      /// InverseAngstromSq
      static const UnitLabel InverseAngstromSq;
      /// MilliElectronVolts
      static const UnitLabel MilliElectronVolts;
      /// Metre
      static const UnitLabel Metre;
      /// Nanometre
      static const UnitLabel Nanometre;
      /// Inverse centimeters
      static const UnitLabel InverseCM;
    };

  } // namespace Units
}} // namespace Mantid::Kernel

#endif  /* MANTID_KERNEL_UNITLABELTYPES_H_ */
