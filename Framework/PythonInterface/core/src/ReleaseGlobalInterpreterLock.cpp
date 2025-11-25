// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"

namespace Mantid::PythonInterface {

/**
 * Ensures this thread releases the Python GIL also save trace information
 * to be restored upon destruction.
 */
ReleaseGlobalInterpreterLock::ReleaseGlobalInterpreterLock()
    : m_tracefunc(nullptr), m_tracearg(nullptr), m_saved(nullptr) {
  PyThreadState const *curThreadState = PyThreadState_GET();
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

} // namespace Mantid::PythonInterface
