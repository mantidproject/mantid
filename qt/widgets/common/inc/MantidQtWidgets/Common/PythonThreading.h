#ifndef PYTHONTHREADING_H_
#define PYTHONTHREADING_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/PythonSystemHeader.h" // this needs to go first

//------------------------------------------------------------------------------
// Python Interpreter
//------------------------------------------------------------------------------
class EXPORT_OPT_MANTIDQT_COMMON PythonInterpreter {
public:
  static void initialize();
  static void finalize();
};

//------------------------------------------------------------------------------
// PythonGIL
//------------------------------------------------------------------------------
/**
 * Defines a structure for acquiring/releasing the Python GIL
 * using the RAII pattern. Modeled after QMutex
 *
 */
class EXPORT_OPT_MANTIDQT_COMMON PythonGIL {

public:
  static bool locked();

public:
  PythonGIL();
  PythonGIL(const PythonGIL &) = delete;
  PythonGIL &operator=(const PythonGIL &) = delete;

  void acquire();
  void release();

private:
  PyGILState_STATE m_state;
};

//------------------------------------------------------------------------------
// ScopedInterpreterLock
//------------------------------------------------------------------------------

/**
 * Acquires a lock in the constructor and releases it in the destructor.
 * @tparam T Templated on the lock type
 */
template <typename T> class ScopedGIL {
public:
  ScopedGIL() : m_lock() { m_lock.acquire(); }
  ~ScopedGIL() { m_lock.release(); }

private:
  T m_lock;
};

/// Typedef for scoped lock
using ScopedPythonGIL = ScopedGIL<PythonGIL>;

#endif /* PYTHONTHREADING_H_ */
