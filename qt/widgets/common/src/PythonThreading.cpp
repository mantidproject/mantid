//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtWidgets/Common/PythonThreading.h"
#include <iostream>

#include <QThread>

//------------------------------------------------------------------------------
// PythonGIL Public members
//------------------------------------------------------------------------------

/**
 * Leaves the lock unlocked. You are strongly encouraged to use
 * the ScopedInterpreterLock class to control this
 */
PythonGIL::PythonGIL() : m_state(PyGILState_UNLOCKED), m_acquired(false) {}

/**
 * Calls PyGILState_Ensure. A call to this must be matched by a call to release
 * on the same thread.
 */
void PythonGIL::acquire() {
  m_state = PyGILState_Ensure();
  m_acquired = true;
}

/**
 * Calls PyGILState_Release
 */
void PythonGIL::release() {
  PyGILState_Release(m_state);
  m_acquired = false;
}

//------------------------------------------------------------------------------
// RecursivePythonGIL public members
//------------------------------------------------------------------------------
/**
 * Leaves the lock unlocked. You are strongly encouraged to use
 * the ScopedRecursiveInterpreterLock class to control this
 */
RecursivePythonGIL::RecursivePythonGIL() : m_count(0), m_lock() {}

void RecursivePythonGIL::acquire() {
  if (m_count == 0) {
    m_lock.acquire();
  }
  m_count += 1;
}

void RecursivePythonGIL::release() {
  if (--m_count == 0) {
    m_lock.release();
  }
}
