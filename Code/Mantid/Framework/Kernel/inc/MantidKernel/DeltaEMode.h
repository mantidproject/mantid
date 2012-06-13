#ifndef MANTID_KERNEL_DELTAEMODE_H_
#define MANTID_KERNEL_DELTAEMODE_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
      /// Define the available energy transfer modes
      enum Type { Elastic, Direct, Indirect };
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
