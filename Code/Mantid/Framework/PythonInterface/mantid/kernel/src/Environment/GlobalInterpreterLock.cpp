#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"

namespace Mantid {
namespace PythonInterface {
namespace Environment {

//------------------------------------------------------------------------------
// GlobalInterpreterLock Static helpers
//------------------------------------------------------------------------------

/**
 * Only if the current thread has no threadstate. There must be a matching
 * call to release() or a deadlock could ensue.
 * @param tstate Filled by the return PyGILState_Ensure(). Only depend on this
 * if this function returns true
 * @return True if the lock was acquired, false otherwise
 */
PyGILState_STATE GlobalInterpreterLock::acquire() {
  return PyGILState_Ensure();
}

/**
 * There must be have been a call to acquire() to create the tstate value given here.
 * @param tstate The thread-state returned by acquire()
 */
void GlobalInterpreterLock::release(PyGILState_STATE tstate) {
  PyGILState_Release(tstate);
}

//------------------------------------------------------------------------------
// GlobalInterpreterLock Public members
//------------------------------------------------------------------------------

/**
 * Calls PyGILState_Ensure()
 */
GlobalInterpreterLock::GlobalInterpreterLock()
  : m_state(this->acquire()) {
}

/**
 * Calls PyGILState_Release
 */
GlobalInterpreterLock::~GlobalInterpreterLock() {
  this->release(m_state);
}

}
}
}
