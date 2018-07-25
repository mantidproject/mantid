#include "MantidQtWidgets/MplCpp/PythonErrors.h"
#include "MantidQtWidgets/MplCpp/PythonObject.h"
#include "MantidQtWidgets/Common/PythonThreading.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {

/**
 * Unroll a traceback as a string reprsentation and append it to the string
 * @param msg A string to build up the message
 * @param traceback A Python traceback object
 * @param root True if this is the root of the traceback, false otherwise
 */
void tracebackToString(std::string &msg, PyTracebackObject *traceback,
                       bool root = true) {
  if (traceback == nullptr)
    return;
  msg.append("\n  ");
  if (root)
    msg.append("at");
  else
    msg.append("caused by");

  msg.append(" line ")
      .append(std::to_string(traceback->tb_lineno))
      .append(" in \'")
      .append(TO_CSTRING(traceback->tb_frame->f_code->co_filename))
      .append("\'");
  tracebackToString(msg, traceback->tb_next, false);
}
}

/**
 * Construct an exception, converting the current Python error
 * to a string
 * @param withTrace If true include a traceback in the error string
 */
PythonError::PythonError(bool withTrace) : m_msg(errorToString(withTrace)) {}

/**
 * Turn the current global error indicator into a string. If there is
 * no error then an empty string is returned. The error indicator
 * is cleared.
 * @param withTrace If true then include a traceback with the error
 * @return A string representation of the current error
 */
std::string errorToString(bool withTrace) {
  ScopedPythonGIL gil;
  PyObject *exception(nullptr), *value(nullptr), *traceback(nullptr);
  PyErr_Fetch(&exception, &value, &traceback);
  assert(exception);
  PyErr_NormalizeException(&exception, &value, &traceback);
  PyErr_Clear();
  PyObject *strRepr = PyObject_Str(value);
  std::string error;
  if (value && strRepr) {
    error.append(TO_CSTRING(strRepr));
  } else {
    error.append("Unknown exception has occurred.");
  }
  if (withTrace) {
    tracebackToString(error, reinterpret_cast<PyTracebackObject *>(traceback));
  }

  // Ensure we decrement the reference count on the traceback and exception
  // objects as they hold references to local the stack variables that were
  // present when the exception was raised. This could include child algorithms
  // with workspaces stored that would not otherwise be cleaned up until the
  // program exited.
  Py_XDECREF(traceback);
  Py_XDECREF(exception);
  Py_XDECREF(value);
  return error;
}
}
}
}
