#ifndef MANTID_PYTHONINTERFACE_GLOBALINTERPRETERLOCK_H_
#define MANTID_PYTHONINTERFACE_GLOBALINTERPRETERLOCK_H_
/**
    Defines an RAII class for dealing with the Python GIL in non-python
    created C-threads

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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
#include "MantidPythonInterface/kernel/DllConfig.h"
#include <boost/python/detail/wrap_python.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Environment {

/**
 * Defines a structure for acquiring/releasing the Python GIL
 * using the RAII pattern.
 *
 * This class is copied in
 * MantidQt/API/inc/MantidQtAPI/PythonThreading.h as we have no good
 * way to share code with that without tight coupling the GUI layer
 * to PythonInterface
 */
class PYTHON_KERNEL_DLL GlobalInterpreterLock {
public:
  /// @name Static Helpers
  ///@{
  /// Call PyGILState_Ensure
  static PyGILState_STATE acquire();
  /// Call PyGILState_Release
  static void release(PyGILState_STATE tstate);
  ///@}

  /// Default constructor
  GlobalInterpreterLock();
  /// Destructor
  ~GlobalInterpreterLock();

private:
  GlobalInterpreterLock(const GlobalInterpreterLock &);
  /// Current GIL state
  PyGILState_STATE m_state;
};
} // namespace Environment
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_GLOBALINTERPRETERLOCK_H_ */
