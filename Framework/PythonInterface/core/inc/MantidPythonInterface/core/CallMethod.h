// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_CALLMETHOD_H_
#define MANTID_PYTHONINTERFACE_CALLMETHOD_H_

#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapperHelpers.h"

#include <boost/python/call_method.hpp>
#include <boost/python/class.hpp>

namespace Mantid {
namespace PythonInterface {

/// Defines an exception for an undefined attribute
struct UndefinedAttributeError {
  virtual ~UndefinedAttributeError() = default;
};

namespace detail {
/**
 * Wrapper around boost::python::call_method. If the call raises a Python error
 * then this is translated to a C++ exception object inheriting from
 * std::exception or std::runtime_error depending on the type of Python error.
 *
 * Note that this is an implementation method that does not hold the GIL and
 * is only intended to be used below.
 * @param obj Pointer to Python object
 * @param methodName Name of the method call
 * @param args A list of arguments to forward to call_method
 */
template <typename ReturnType, typename... Args>
ReturnType callMethodImpl(PyObject *obj, const char *methodName,
                          const Args &... args) {
  try {
    return boost::python::call_method<ReturnType, Args...>(obj, methodName,
                                                           args...);
  } catch (boost::python::error_already_set &) {
    PyObject *exception = PyErr_Occurred();
    assert(exception);
    if (PyErr_GivenExceptionMatches(exception, PyExc_RuntimeError)) {
      throw PythonRuntimeError();
    } else {
      throw PythonException();
    }
  }
}
} // namespace detail

/**
 * Wrapper around boost::python::call_method to acquire GIL for duration
 * of call. If the call raises a Python error then this is translated to
 * a C++ exception object inheriting from std::exception or std::runtime_error
 * depending on the type of Python error.
 * @param obj Pointer to Python object
 * @param methodName Name of the method call
 * @param args A list of arguments to forward to call_method
 */
template <typename ReturnType, typename... Args>
ReturnType callMethodNoCheck(PyObject *obj, const char *methodName,
                             const Args &... args) {
  GlobalInterpreterLock gil;
  return detail::callMethodImpl<ReturnType, Args...>(obj, methodName, args...);
}

/**
 * Wrapper around boost::python::call_method to acquire GIL for duration
 * of call. If the attribute does not exist then an UndefinedAttributeError
 * is thrown, if the call raises a Python error then this is translated to
 * a C++ exception object inheriting from std::exception or std::runtime_error
 * depending on the type of Python error.
 * @param obj Pointer to Python object
 * @param methodName Name of the method call
 * @param args A list of arguments to forward to call_method
 */
template <typename ReturnType, typename... Args>
ReturnType callMethod(PyObject *obj, const char *methodName,
                      const Args &... args) {
  GlobalInterpreterLock gil;
  if (typeHasAttribute(obj, methodName)) {
    return detail::callMethodImpl<ReturnType, Args...>(obj, methodName,
                                                       args...);
  } else {
    throw UndefinedAttributeError();
  }
}
} // namespace PythonInterface
} // namespace Mantid

#endif // MANTID_PYTHONINTERFACE_CALLMETHOD_H_
