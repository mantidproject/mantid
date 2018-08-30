#ifndef MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETERLOCK_H_
#define MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETERLOCK_H_

#include "MantidPythonInterface/kernel/DllConfig.h"
#include <boost/python/detail/wrap_python.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Environment {

/**
 * Defines a structure for releaseing the Python GIL
 * using the RAII pattern. This releases the Python GIL
 * for the duration of the current scope.
 */
class PYTHON_KERNEL_DLL ReleaseGlobalInterpreterLock {
public:
  /// Default constructor
  ReleaseGlobalInterpreterLock();
  /// Destructor
  ~ReleaseGlobalInterpreterLock();

private:
  // Stores the current python trace used to track where in
  // a python script you are.
  Py_tracefunc m_tracefunc;
  PyObject *m_tracearg;
  /// Saved thread state
  PyThreadState *m_saved;
};

} // namespace Environment
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETERLock_H_ */
