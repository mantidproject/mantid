//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtAPI/PythonThreading.h"
#include <QThread>

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

//------------------------------------------------------------------------------
// PyGILStateService static helpers
//------------------------------------------------------------------------------
/**
 * @param targetStore The PyGILStateService object to store the state
 */
void PyGILStateService::acquireAndStore(PyGILStateService &targetStore) {
  if(!targetStore.contains(QThread::currentThread())) {
    targetStore.add(QThread::currentThread(), GlobalInterpreterLock::acquire());
  }
}

/**
 * @param targetStore The PyGILStateService object from which to relesae
 * the state
 * @param targetStore The PyGILStateService object storing the state
 */
void PyGILStateService::dropAndRelease(PyGILStateService &targetStore) {
  if(targetStore.contains(QThread::currentThread())) {
    GlobalInterpreterLock::release(targetStore.take(QThread::currentThread()));
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
 * A call to add() must have been performed for the given thread
 * @param thread A pointer to a QThread whose tstate should be retrieved. If found, the
 * state is dropped from the map.
 * @return tstate PyGILState_STATE value associated with the given thread
 */
PyGILState_STATE PyGILStateService::take(QThread* thread) {
  return m_mapping.take(thread);
}
