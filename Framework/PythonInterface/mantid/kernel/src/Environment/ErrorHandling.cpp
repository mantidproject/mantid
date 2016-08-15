//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"

#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>
#include <frameobject.h> //Python

#include <sstream>
#include <stdexcept>

using boost::python::extract;
using boost::python::object;

namespace Mantid {
namespace PythonInterface {
namespace Environment {
namespace {
void tracebackToMsg(std::stringstream &msg, PyTracebackObject *traceback,
                    bool root = true) {
  if (traceback == nullptr)
    return;
  msg << "\n  ";
  if (root)
    msg << "at";
  else
    msg << "caused by";

  msg << " line " << traceback->tb_lineno << " in \'"
      << extract<const char *>(traceback->tb_frame->f_code->co_filename)()
      << "\'";
  tracebackToMsg(msg, traceback->tb_next, false);
}
}

/**
 * Construct an exception object where the message is populated from the
 * current Python exception state
 * @param withTrace If true, include the full traceback in the message
 */
PythonException::PythonException(bool withTrace) : std::exception(), m_msg() {
  GlobalInterpreterLock gil;
  if (!PyErr_Occurred()) {
    throw std::runtime_error(
        "PythonException thrown but no Python error state set!");
  }
  PyObject *exception(nullptr), *value(nullptr), *traceback(nullptr);
  PyErr_Fetch(&exception, &value, &traceback);
  PyErr_NormalizeException(&exception, &value, &traceback);
  PyErr_Clear();
  PyObject *strRepr = PyObject_Str(value);
  std::stringstream builder;
  if (value && strRepr) {
    builder << extract<const char *>(strRepr)();
  } else {
    builder << "Unknown exception has occurred.";
  }
  if (withTrace) {
    tracebackToMsg(builder, reinterpret_cast<PyTracebackObject *>(traceback));
  }

  // Ensure we decrement the reference count on the traceback and exception
  // objects as they hold references to local the stack variables that were
  // present when the exception was raised. This could include child algorithms
  // with workspaces stored that would not otherwise be cleaned up until the
  // program exited.
  Py_XDECREF(traceback);
  Py_XDECREF(exception);
  Py_XDECREF(value);
  m_msg = builder.str();
}
}
}
}
