// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/DllConfig.h"
#include "MantidPythonInterface/core/WrapPython.h"

namespace Mantid {
namespace PythonInterface {

struct MANTID_PYTHONINTERFACE_CORE_DLL GILState {
  PyGILState_STATE m_state;
};

/**
 * Defines a structure for acquiring/releasing the Python GIL
 * using the RAII pattern.
 */
class MANTID_PYTHONINTERFACE_CORE_DLL GlobalInterpreterLock {
public:
  /// @name Static Helpers
  ///@{
  /// Check state of lock
  static bool locked();
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
  GlobalInterpreterLock(const GlobalInterpreterLock &);
  /// Current GIL state
  PyGILState_STATE m_state;
};

} // namespace PythonInterface
} // namespace Mantid
