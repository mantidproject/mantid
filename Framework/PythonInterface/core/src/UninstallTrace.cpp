// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/UninstallTrace.h"

namespace Mantid::PythonInterface {

/**
 * Saves any function and argument previously set by PyEval_SetTrace
 * and calls PyEval_SetTrace(nullptr, nullptr) to remove the trace function
 */
UninstallTrace::UninstallTrace() {
  PyThreadState const *curThreadState = PyThreadState_GET();
  m_tracefunc = curThreadState->c_tracefunc;
  m_tracearg = curThreadState->c_traceobj;
  Py_XINCREF(m_tracearg);
  PyEval_SetTrace(nullptr, nullptr);
}

/**
 * Reinstates any trace function with PyEval_SetTrace and any saved arguments
 * from the constructor
 */
UninstallTrace::~UninstallTrace() { PyEval_SetTrace(m_tracefunc, m_tracearg); }

} // namespace Mantid::PythonInterface
