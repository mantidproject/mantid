#ifndef PYTHONTHREADING_H_
#define PYTHONTHREADING_H_

#include "MantidQtAPI/PythonSystemHeader.h"

/**
 * Defines a structure for acquiring/releasing the Python GIL
 * using the RAII pattern
 */
class GlobalInterpreterLock
{
public:
  /// Default constructor
  GlobalInterpreterLock();
  /// Destructor
  ~GlobalInterpreterLock();
  /// Is this thread the active thread
  static bool thisPyThreadIsActive();

private:
  GlobalInterpreterLock(const GlobalInterpreterLock&);
  /// Did we choose to acquire the lock
  bool m_locked;
  /// Current GIL state
  PyGILState_STATE m_state;
};

#endif /* PYTHONTHREADING_H_ */
