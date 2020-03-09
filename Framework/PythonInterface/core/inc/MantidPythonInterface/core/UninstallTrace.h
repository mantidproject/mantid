// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_UNINSTALLTRACE_H
#define MANTID_PYTHONINTERFACE_UNINSTALLTRACE_H

#include "MantidPythonInterface/core/DllConfig.h"
#include "MantidPythonInterface/core/WrapPython.h"

namespace Mantid::PythonInterface {

/**
 * @brief RAII handler to temporarily remove and reinstall a Python trace
 * function
 */
class MANTID_PYTHONINTERFACE_CORE_DLL UninstallTrace {
public:
  UninstallTrace();
  ~UninstallTrace();

private:
  Py_tracefunc m_tracefunc;
  PyObject *m_tracearg;
};

} // namespace Mantid::PythonInterface

#endif // MANTID_PYTHONINTERFACE_UNINSTALLTRACE_H
