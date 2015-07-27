#ifndef PYTHONTHREADING_H_
#define PYTHONTHREADING_H_

#include "MantidQtAPI/PythonSystemHeader.h" // this needs to go first
#include "MantidQtAPI/DllOption.h"
#include <QHash>

//------------------------------------------------------------------------------
// GlobalInterpreterLock
//------------------------------------------------------------------------------
/**
 * Defines a structure for acquiring/releasing the Python GIL
 * using the RAII pattern.
 *
.* This is an exact copy of the one in
 * Framework/PythonInterface/inc/MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h
 * as we have no good way to share code with that without tight coupling the GUI layer
 * to PythonInterface
 */
class EXPORT_OPT_MANTIDQT_API GlobalInterpreterLock
{
public:
  /// @name Static Helpers
  ///@{
  /// Call PyGILState_Ensure
  static PyGILState_STATE acquire();
  /// Call PyGILState_Release
  static void release(PyGILState_STATE tstate);
  ///@}

  /// Default constructor
  GlobalInterpreterLock();
  /// Destructor
  ~GlobalInterpreterLock();

private:
  GlobalInterpreterLock(const GlobalInterpreterLock&);
  /// Current GIL state
  PyGILState_STATE m_state;
};

//------------------------------------------------------------------------------
// PyGILStateService
//------------------------------------------------------------------------------

/**
 * Defines an index for storing PyGILState_STATE values returned
 * from PyGILState_Ensure calls.
 */
class EXPORT_OPT_MANTIDQT_API PyGILStateService {
public:
  ///@name Static helpers
  ///@{
  /// Acquire lock for this thread and hold the lock in the given service
  static void acquireAndStore(PyGILStateService &targetStore);
    /// Release lock for this thread drop the lock state from the service
  static void dropAndRelease(PyGILStateService &targetStore);
  ///@}

  /// Return true if the given thread contains an assoicated value in the index
  bool contains(QThread *thread) const { return m_mapping.contains(thread); }
  /// Associate a QThread with a PyGILState_STATE value
  void add(QThread* thread, PyGILState_STATE tstate);
  /// Find the PyGILState for the given QThread
  PyGILState_STATE take(QThread* thread);

private:
  typedef QHash<QThread*, PyGILState_STATE> QThreadToGILMap;
  QThreadToGILMap m_mapping;
};


#endif /* PYTHONTHREADING_H_ */
