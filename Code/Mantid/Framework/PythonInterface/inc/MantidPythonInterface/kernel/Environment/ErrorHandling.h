#ifndef MANTID_PYTHONINTERFACE_ERRORHANDLING_H
#define MANTID_PYTHONINTERFACE_ERRORHANDLING_H
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

    File change history is stored at: <https://github.com/mantidproject/mantid>    
*/
#include "MantidKernel/System.h"

/**
 * This file defines error handling code that transforms
 * a Python error state to C++ exceptions.
 */

namespace Mantid { namespace PythonInterface {
  namespace Environment
  {
    /// Convert Python error state to C++ exception
    DLLExport void throwRuntimeError(const bool withTrace = true);
  }
}}



#endif /* MANTID_PYTHONINTERFACE_CALLMETHOD_H_ */
