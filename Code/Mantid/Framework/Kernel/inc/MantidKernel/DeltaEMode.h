#ifndef MANTID_KERNEL_DELTAEMODE_H_
#define MANTID_KERNEL_DELTAEMODE_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid
{
  namespace Kernel
  {
    /**
     * Defines the possible energy transfer modes:
     *   - Elastic
     *   - Direct
     *   - Indirect
     * and functions to convert to/from strings.
     * It also returns a list of the available modes
     */
    struct MANTID_KERNEL_DLL DeltaEMode
    {
      /** Define the available energy transfer modes
       *  It is important to assign enums proper numbers, until direct correspondence between enums and their emodes 
       *  used by the external units conversion algorithms within the Mantid, so the agreement should be the stame        */
      enum Type 
      { Elastic =0, 
        Direct  =1, 
        Indirect=2,
        Undefined //< The type for the situations, where instrument can not be reasonably defined (e.g.  ws with detector information lost)
                            /// this mode should not be displayed among modes availible to select but may have string representation
      };
      /// Return a string representation of the given mode
      static std::string asString(const Type mode);
      /// Returns the emode from the given string
      static Type fromString(const std::string & modeStr);
      /// Returns the string list of available modes
      static const std::vector<std::string> availableTypes();
    };
  }
}

#endif /* MANTID_KERNEL_DELTAEMODE_H_ */
