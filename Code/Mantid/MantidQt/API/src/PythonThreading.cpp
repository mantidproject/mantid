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
// GlobalInterpreterLock Static helpers
//------------------------------------------------------------------------------

/// @return True if this thread considered the active Python thread
bool GlobalInterpreterLock::pyThreadIsActive() {
  PyThreadState *tstate = _PyThreadState_Current;
  return tstate && (tstate == PyGILState_GetThisThreadState());
}

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
 * If there is no current Python threadstate then create one and acquire the GIL. Essentially
 * just calls PyGILState_Ensure()
 */
GlobalInterpreterLock::GlobalInterpreterLock()
  : m_locked(false), m_state(PyGILState_UNLOCKED) {
  // The current value of m_state is wrong but I have to set it to something. We can't just
  // rely on m_state to know what we have done so we have the locked boolean too
  if(!pyThreadIsActive()) {
    m_state = acquire();
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
// PyGILStateService static helpers
//------------------------------------------------------------------------------
/**
 * @param targetStore The PyGILStateService object to store the state
 */
void PyGILStateService::acquireAndStore(PyGILStateService &targetStore) {
  if(!GlobalInterpreterLock::pyThreadIsActive()) {
    targetStore.add(QThread::currentThread(), GlobalInterpreterLock::acquire());
  }
}

/**
 * @param targetStore The PyGILStateService object from which to relesae
 * the state
 * @param targetStore The PyGILStateService object storing the state
 */
void PyGILStateService::dropAndRelease(PyGILStateService &targetStore) {
  PyGILState_STATE tstate(PyGILState_UNLOCKED);
  if(targetStore.retrieve(QThread::currentThread(), tstate)) {
    GlobalInterpreterLock::release(tstate);
  }
}

//------------------------------------------------------------------------------
// PyGILStateService public members
//------------------------------------------------------------------------------
/**
 * @param thread A pointer to the current QThread object that defines a thread
 * @param tstate The value of PyGILState provided by a call to PyGILState_Ensure()
 */
void PyGILStateService::add(QThread* thread, PyGILState_STATE tstate) {
  m_mapping.insert(thread, tstate);
}

/**
 * @param thread A pointer to a QThread whose tstate should be retrieved. If found, the
 * state is dropped from the map
 * @return True if the threadstate was found and dropped, false otherwise
 */
bool PyGILStateService::retrieve(QThread* thread, PyGILState_STATE &tstate) {
  auto iter = m_mapping.find(thread);
  if (iter != m_mapping.end()) {
    tstate = *iter;
    m_mapping.erase(iter);
    return true;
  }
  else {
    return false;
  }
}
