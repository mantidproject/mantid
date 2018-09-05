#include "MantidPythonInterface/core/GlobalInterpreterLock.h"

namespace Mantid {
namespace PythonInterface {

//------------------------------------------------------------------------------
// GlobalInterpreterLock Static helpers
//------------------------------------------------------------------------------

/**
 * Check if the current thread has the lock
 * @return True if the current thread holds the GIL, false otherwise
 */
bool GlobalInterpreterLock::locked() {
#if PY_VERSION_HEX < 0x03000000
  PyThreadState *ts = _PyThreadState_Current;
  return (ts && ts == PyGILState_GetThisThreadState());
#else
  return (PyGILState_Check() == 1);
#endif
}

/**
 * @return A handle to the Python threadstate before the acquire() call.
 */
PyGILState_STATE GlobalInterpreterLock::acquire() {
  return PyGILState_Ensure();
}

/**
 * There must be have been a call to acquire() to create the tstate value given
 * here.
 * @param tstate The Python threadstate returned by the matching call to
 * acquire()
 */
void GlobalInterpreterLock::release(PyGILState_STATE tstate) {
  PyGILState_Release(tstate);
}

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

} // namespace PythonInterface
} // namespace Mantid
