#ifndef MANTID_PYTHONINTERFACE_CALLMETHOD_H_
#define MANTID_PYTHONINTERFACE_CALLMETHOD_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
*/
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/call_method.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Environment {

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
}

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
  Environment::GlobalInterpreterLock gil;
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
  if (Environment::typeHasAttribute(obj, methodName)) {
    return detail::callMethodImpl<ReturnType, Args...>(obj, methodName,
                                                       args...);
  } else {
    throw UndefinedAttributeError();
  }
}
}
}
}

#endif // MANTID_PYTHONINTERFACE_CALLMETHOD_H_
