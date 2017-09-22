//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtWidgets/Common/PythonThreading.h"

namespace {
PyThreadState *INITIAL_TS = nullptr;
}

//------------------------------------------------------------------------------
// PythonInterpreter Public members
//------------------------------------------------------------------------------
void PythonInterpreter::initialize() {
  Py_Initialize();
  PyEval_InitThreads();
  // Release GIL
  INITIAL_TS = PyEval_SaveThread();
}

// Finalize the Python process. The GIL must be held to
// call this function but after it completes calling
// any python functions is undefined behaviour
void PythonInterpreter::finalize() { Py_Finalize(); }

//------------------------------------------------------------------------------
// PythonGIL Public members
//------------------------------------------------------------------------------

/**
 * Check if the current thread has the lock
 * @return True if the current thread holds the GIL, false otherwise
 */
bool PythonGIL::locked() {
#if PY_VERSION_HEX < 0x03000000
  PyThreadState *ts = _PyThreadState_Current;
  return (ts && ts == PyGILState_GetThisThreadState());
#else
  return (PyGILState_Check() == 1);
#endif
}

/**
 * Leaves the lock unlocked. You are strongly encouraged to use
 * the ScopedInterpreterLock class to control this
 */
PythonGIL::PythonGIL() : m_state(PyGILState_UNLOCKED) {}

/**
 * Calls PyGILState_Ensure. A call to this must be matched by a call to release
 * on the same thread.
 */
void PythonGIL::acquire() { m_state = PyGILState_Ensure(); }

/**
 * Calls PyGILState_Release
 */
void PythonGIL::release() { PyGILState_Release(m_state); }
