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
 * Constructor.
 */
GlobalInterpreterLock::GlobalInterpreterLock()
  : m_locked(false), m_state(PyGILState_UNLOCKED) {
  // The current value of m_state is wrong but I have to set it to something. We can't just
  // rely on m_state to know what we have done so we have the locked boolean too
  if(!thisPyThreadIsActive()) {
    m_state = PyGILState_Ensure();
    m_locked = true;
  }
}


/**
 * Release the GIL for this thread if we acquired it
 */
GlobalInterpreterLock::~GlobalInterpreterLock() {
  if(m_locked) {
    PyGILState_Release(m_state);
  }
}

/// Returns True if this thread considered the active thread
bool GlobalInterpreterLock::thisPyThreadIsActive() {
  PyThreadState *tstate = _PyThreadState_Current;
  return tstate && (tstate == PyGILState_GetThisThreadState());
}
