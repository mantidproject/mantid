#ifndef PYTHONTHREADING_H_
#define PYTHONTHREADING_H_

#include "MantidQtAPI/PythonSystemHeader.h"
#include <QHash>

//------------------------------------------------------------------------------
// GlobalInterpreterLock
//------------------------------------------------------------------------------
/**
 * Defines a structure for acquiring/releasing the Python GIL
 * using the RAII pattern
 */
class GlobalInterpreterLock
{
public:
  /// @name Static Helpers
  ///@{
  /// Is this thread the active thread
  static bool pyThreadIsActive();
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
  /// Did we choose to acquire the lock
  bool m_locked;
  /// Current GIL state
  PyGILState_STATE m_state;
};

//------------------------------------------------------------------------------
// PyGILStateService
//------------------------------------------------------------------------------

/**
 * Defines an index for mapping QThreads to their
 * respective PyGILState_STATE values
 */
class PyGILStateService {
public:
  ///@name Static helpers
  ///@{
  /// Acquire lock for this thread and hold the lock in the given service
  static void acquireAndStore(PyGILStateService &targetStore);
    /// Release lock for this thread drop the lock state from the service
  static void dropAndRelease(PyGILStateService &targetStore);
  ///@}

  /**
   * @param thread A pointer to the current QThread object that defines a thread
   * @param tstate The value of PyGILState provided by a call to PyGILState_Ensure()
   */
  void add(QThread* thread, PyGILState_STATE tstate);
  /**
   * @param thread A pointer to a QThread whose tstate should be retrieved. If found, the
   * state is dropped from the map.
   * @param tstate Filled PyGILState_STATE value associated with this thread if found
   * @return True if a value was found and removed
   */
  bool retrieve(QThread* thread, PyGILState_STATE &tstate);

private:
  typedef QHash<QThread*, PyGILState_STATE> QThreadToGILMap;
  QThreadToGILMap m_mapping;
};


#endif /* PYTHONTHREADING_H_ */
