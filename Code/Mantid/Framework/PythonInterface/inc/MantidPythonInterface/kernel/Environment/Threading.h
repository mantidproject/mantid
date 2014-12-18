#ifndef MANTID_PYTHONINTERFACE_PYTHONTHREADING_H_
#define MANTID_PYTHONINTERFACE_PYTHONTHREADING_H_
/**
    Defines utility functions and classes for dealing with Python threads

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include <boost/python/detail/wrap_python.hpp>

namespace Mantid { namespace PythonInterface {
  namespace Environment
  {

    /// Saves a pointer to the PyThreadState of the main thread
    void saveMainThreadState(PyThreadState *threadState);

    /**
     * Defines a structure for creating and destroying a
     * Python thread state using the RAII pattern
     */
    struct PythonThreadState
    {
      explicit PythonThreadState();
      ~PythonThreadState();
    private:
      PythonThreadState(const PythonThreadState&);
      PythonThreadState& operator=(const PythonThreadState&);

      PyThreadState * m_mainThreadState;
      PyThreadState * m_thisThreadState;
    };

    /**
     * Defines a structure for acquiring/releasing the Python GIL
     * using the RAII pattern
     */
    struct GlobalInterpreterLock
    {
      GlobalInterpreterLock() : m_state(PyGILState_Ensure())
      {}

      ~GlobalInterpreterLock()
      {
        PyGILState_Release(m_state);
      }
    private:
      /// Current GIL state
      PyGILState_STATE m_state;
    };
  }
}}


#endif /* MANTID_PYTHONAPI_PYTHONTHREADING_H_ */
