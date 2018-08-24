#include "MantidPythonInterface/kernel/Environment/ReleaseGlobalInterpreterLock.h"

namespace Mantid {
namespace PythonInterface {
namespace Environment {

/**
 * Ensures this thread releases the Python GIL also save trace information
 * to be restored upon destruction.
 */
ReleaseGlobalInterpreterLock::ReleaseGlobalInterpreterLock()
    : m_tracefunc(nullptr), m_tracearg(nullptr), m_saved(nullptr) {
  PyThreadState *curThreadState = PyThreadState_GET();
  m_tracefunc = curThreadState->c_tracefunc;
  m_tracearg = curThreadState->c_traceobj;
  Py_XINCREF(m_tracearg);
  PyEval_SetTrace(nullptr, nullptr);
  m_saved = PyEval_SaveThread();
}

/**
 * Restores the Python GIL to the thread when the object falls out of scope.
 */
ReleaseGlobalInterpreterLock::~ReleaseGlobalInterpreterLock() {
  PyEval_RestoreThread(m_saved);
  PyEval_SetTrace(m_tracefunc, m_tracearg);
  Py_XDECREF(m_tracearg);
}

} // namespace Environment
} // namespace PythonInterface
} // namespace Mantid