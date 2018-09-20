#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"

namespace Mantid {
namespace PythonInterface {
namespace Environment {

//------------------------------------------------------------------------------
// GlobalInterpreterLock Static helpers
//------------------------------------------------------------------------------

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
} // namespace Environment
} // namespace PythonInterface
} // namespace Mantid
