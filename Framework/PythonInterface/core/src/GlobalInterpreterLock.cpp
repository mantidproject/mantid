// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/VersionCompat.h"

namespace Mantid::PythonInterface {

//------------------------------------------------------------------------------
// GlobalInterpreterLock Static helpers
//------------------------------------------------------------------------------

/**
 * Check if the current thread has the lock
 * @return True if the current thread holds the GIL, false otherwise
 */
bool GlobalInterpreterLock::locked() {
#if defined(IS_PY3K)
  return (PyGILState_Check() == 1);
#else
  PyThreadState const *ts = _PyThreadState_Current;
  return (ts && ts == PyGILState_GetThisThreadState());
#endif
}

/**
 * @return A handle to the Python threadstate before the acquire() call.
 */
PyGILState_STATE GlobalInterpreterLock::acquire() { return PyGILState_Ensure(); }

/**
 * There must be have been a call to acquire() to create the tstate value given
 * here.
 * @param tstate The Python threadstate returned by the matching call to
 * acquire()
 */
void GlobalInterpreterLock::release(PyGILState_STATE tstate) { PyGILState_Release(tstate); }

//------------------------------------------------------------------------------
// GlobalInterpreterLock Public members
//------------------------------------------------------------------------------

/**
 * Ensures this thread is ready to call Python code
 */
GlobalInterpreterLock::GlobalInterpreterLock() : m_state(this->acquire()) {}

/**
 * Resets the Python threadstate to the state before
 * this object was created.
 */
GlobalInterpreterLock::~GlobalInterpreterLock() { this->release(m_state); }

} // namespace Mantid::PythonInterface
