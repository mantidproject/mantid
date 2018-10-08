// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_ERRORHANDLING_H
#define MANTID_PYTHONINTERFACE_ERRORHANDLING_H

#include "MantidPythonInterface/core/DllConfig.h"
#include <stdexcept>
#include <string>

/**
 * This file defines error handling code that transforms
 * a Python error state to C++ exceptions.
 */
namespace Mantid {
namespace PythonInterface {

/**
 * Exception type that captures the current Python error state
 * as a generic C++ exception for any general Python exception
 */
class MANTID_PYTHONINTERFACE_CORE_DLL PythonException : public std::exception {
public:
  PythonException(bool withTrace = true);

  const char *what() const noexcept override { return m_msg.c_str(); }

private:
  std::string m_msg;
};

/// Exception type that captures the current Python error state
/// as a C++ std::runtime exception
class MANTID_PYTHONINTERFACE_CORE_DLL PythonRuntimeError
    : public std::runtime_error {
public:
  PythonRuntimeError(bool withTrace = true);
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_ERRORHANDLING_H_ */
