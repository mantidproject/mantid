#ifndef MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETER_H_
#define MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETER_H_

#endif /* MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETER_H_ */

#include "MantidPythonInterface/kernel/DllConfig.h"

namespace Mantid {
namespace PythonInterface {
namespace Environment {

/**
 * Defines a structure for releaseing the Python GIL
 * using the RAII pattern. This releases the Python GIL 
 * for the duration of the current scope.
 */
class PYTHON_KERNEL_DLL ReleaseGlobalInterpreter {
public:

  /// Default constructor
  ReleaseGlobalInterpreter();
  /// Destructor
  ~ReleaseGlobalInterpreter();

private:
  // Stores the current python trace used to track where in 
  // a python script you are. 
  Py_tracefunc m_tracefunc;
  PyObject *m_tracearg;
  /// Saved thread state
  PyThreadState *m_saved; 
};

ReleaseGlobalInterpreter::ReleaseGlobalInterpreter()
 : m_tracefunc(nullptr), m_tracearg(nullptr), m_saved(nullptr) {
    PyThreadState *curThreadState = PyThreadState_GET();
    m_tracefunc = curThreadState->c_tracefunc;
    m_tracearg = curThreadState->c_traceobj;
    Py_XINCREF(m_tracearg);
    PyEval_SetTrace(nullptr, nullptr);
    m_saved = PyEval_SaveThread();
}

ReleaseGlobalInterpreter::~ReleaseGlobalInterpreter() {
    PyEval_RestoreThread(m_saved);
    PyEval_SetTrace(m_tracefunc, m_tracearg);
    Py_XDECREF(m_tracearg);
}

}
}
}