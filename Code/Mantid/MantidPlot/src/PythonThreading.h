#ifndef PYTHONTHREADING_H_
#define PYTHONTHREADING_H_

#include "PythonSystemHeader.h"
#include <QObject>

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
  Q_DISABLE_COPY(GlobalInterpreterLock)
  /// Current GIL state
  PyGILState_STATE m_state;
};

#endif /* PYTHONTHREADING_H_ */
