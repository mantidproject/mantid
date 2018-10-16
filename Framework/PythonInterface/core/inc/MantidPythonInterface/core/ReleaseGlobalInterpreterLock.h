// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETERLOCK_H_
#define MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETERLOCK_H_

#include "MantidPythonInterface/core/DllConfig.h"
#include <boost/python/detail/wrap_python.hpp>

namespace Mantid {
namespace PythonInterface {

/**
 * Defines a structure for releasing the Python GIL
 * using the RAII pattern. This releases the Python GIL
 * for the duration of the current scope.
 */
class MANTID_PYTHONINTERFACE_CORE_DLL ReleaseGlobalInterpreterLock {
public:
  /// Default constructor
  ReleaseGlobalInterpreterLock();
  /// Destructor
  ~ReleaseGlobalInterpreterLock();

private:
  Py_tracefunc m_tracefunc;
  PyObject *m_tracearg;
  PyThreadState *m_saved;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_RELEASEGLOBALINTERPRETERLock_H_ */
