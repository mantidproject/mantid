//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtAPI/PythonThreading.h"
#include <QThread>

//------------------------------------------------------------------------------
// helpers
//------------------------------------------------------------------------------
extern "C" {
  /// Links to symbol in the python27 library of the same name
  extern PyThreadState *_PyThreadState_Current;
}

//------------------------------------------------------------------------------
// Public members
//------------------------------------------------------------------------------

/**
 * If there is no current Python threadstate then create one and acquire the GIL. Essentially
 * just calls PyGILState_Ensure()
 */
GlobalInterpreterLock::GlobalInterpreterLock()
  : m_locked(false), m_state(PyGILState_UNLOCKED) {
  // The current value of m_state is wrong but I have to set it to something. We can't just
  // rely on m_state to know what we have done so we have the locked boolean too
  if(!pyThreadIsActive()) {
    m_state = this->acquire();
    m_locked = true;
  }
}

/**
 * If this object acquired the GIL then release it. Basically just calls PyGILState_Release
 */
GlobalInterpreterLock::~GlobalInterpreterLock() {
  if(m_locked) this->release(m_state);
}

//------------------------------------------------------------------------------
// Static helpers
//------------------------------------------------------------------------------

/// @return True if this thread considered the active Python thread
bool GlobalInterpreterLock::pyThreadIsActive() {
  PyThreadState *tstate = _PyThreadState_Current;
  return tstate && (tstate == PyGILState_GetThisThreadState());
}

/**
 * There must be a matching call to release() or a deadlock will ensue.
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
