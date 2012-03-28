#ifndef PYTHONTHREADING_H_
#define PYTHONTHREADING_H_

#include "PythonSystemHeader.h"
#include <QThread>
#include <QCoreApplication>
#include <iostream>

/**
 * Defines a structure for acquiring/releasing the Python GIL
 * using the RAII pattern
 */
struct GlobalInterpreterLock
{
  GlobalInterpreterLock() : m_state(PyGILState_Ensure())
  {}

  ~GlobalInterpreterLock()
  {
    PyGILState_Release(m_state);
  }
private:
  /// Current GIL state
  PyGILState_STATE m_state;
};

/**
 * Defines a structure for creating and destroying a
 * Python thread state using the RAII pattern
 */
struct PythonThreadState
{
  PythonThreadState() : m_mainThreadState(NULL), m_thisThreadState(NULL)
  {}

  explicit PythonThreadState(PyThreadState * mainThreadState)
    : m_mainThreadState(mainThreadState), m_thisThreadState(NULL)
  {
    if(QThread::currentThread() != QCoreApplication::instance()->thread())
    {
      std::cerr << "Creating new python thread from " << QThread::currentThread() << "\n";
      PyEval_AcquireLock();
      PyInterpreterState * mainInterpreterState = m_mainThreadState->interp;
      m_thisThreadState = PyThreadState_New(mainInterpreterState);
      PyThreadState_Swap(m_thisThreadState);
    }
  }

  ~PythonThreadState()
  {
    if(m_thisThreadState)
    {
      PyThreadState_Swap(m_mainThreadState);
      PyThreadState_Clear(m_thisThreadState);
      PyThreadState_Delete(m_thisThreadState);
      PyEval_ReleaseLock();
    }
  }
private:
  PyThreadState * m_mainThreadState;
  PyThreadState * m_thisThreadState;
};

#endif /* PYTHONTHREADING_H_ */
