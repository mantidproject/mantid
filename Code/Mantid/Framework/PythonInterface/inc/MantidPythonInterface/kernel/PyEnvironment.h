#ifndef MANTID_PYTHONINTERFACE_PYENVIRONMENT_H_
#define MANTID_PYTHONINTERFACE_PYENVIRONMENT_H_
/*
  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/System.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct _frame;

namespace Mantid
{
  namespace PythonInterface
  {
    /**
     * The PyEnvironment namespace defines functions that query some aspect
     * of the currently running Python environment
     */
    namespace PyEnvironment
    {
      /// Is the given function name in the call stack
      DLLExport bool isInCallStack(const char * name, _frame* startFrame = NULL);
    }
  }
}



#endif /* MANTID_PYTHONINTERFACE_PYENVIRONMENT_H_ */
