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

/**
 * Wrapper around boost::python::call_method to acquire GIL for duration
 * of call. If the attribute does not exist then an UndefinedAttributeError
 * is thrown, if the call raises a Python error then this is translated to
 * a C++ exception object inheriting from std::exception
 * @param obj Pointer to Python object
 * @param methodName Name of the method call
 * @param args A list of arguments to forward to call_method
 */
template <typename ReturnType, typename... Args>
ReturnType callMethod(PyObject *obj, const char *methodName,
                      const Args &... args) {
  GlobalInterpreterLock gil;
  if (Environment::typeHasAttribute(obj, methodName)) {
    try {
      return boost::python::call_method<ReturnType, Args...>(obj, methodName,
                                                             args...);
    } catch (boost::python::error_already_set &) {
      throw PythonException();
    }
  } else {
    throw UndefinedAttributeError();
  }
}
}
}
}

#endif // MANTID_PYTHONINTERFACE_CALLMETHOD_H_
