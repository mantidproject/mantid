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

namespace Mantid {
namespace PythonInterface {
namespace Environment {
namespace {

/**
 * Unroll a traceback as a string reprsentation and append it to the stream
 * @param msg A stream reference for building up the msg
 * @param traceback A Python traceback object
 * @param root True if this is the root of the traceback, false otherwise
 */
void tracebackToStream(std::ostream &msg, PyTracebackObject *traceback,
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
  tracebackToStream(msg, traceback->tb_next, false);
}

/**
 * Create a string representation of the current Python exception state. Note
 *that
 * the python error state is *not* checked and it is undefined what happens if
 * called in this state.
 *
 * The GIL is held for the duration of this call.
 * @param withTrace If true then provide a full traceback as part of the
 * string message.
 */
std::string exceptionToString(bool withTrace) {
  GlobalInterpreterLock gil;
  PyObject *exception(nullptr), *value(nullptr), *traceback(nullptr);
  PyErr_Fetch(&exception, &value, &traceback);
  assert(exception);
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
    tracebackToStream(builder,
                      reinterpret_cast<PyTracebackObject *>(traceback));
  }

  // Ensure we decrement the reference count on the traceback and exception
  // objects as they hold references to local the stack variables that were
  // present when the exception was raised. This could include child algorithms
  // with workspaces stored that would not otherwise be cleaned up until the
  // program exited.
  Py_XDECREF(traceback);
  Py_XDECREF(exception);
  Py_XDECREF(value);
  return builder.str();
}
} // namespace

// -----------------------------------------------------------------------------
// PythonException
// -----------------------------------------------------------------------------

/**
 * Construct an exception object where the message is populated from the
 * current Python exception state.
 * @param withTrace If true, include the full traceback in the message
 */
PythonException::PythonException(bool withTrace)
    : std::exception(), m_msg(exceptionToString(withTrace)) {}

// -----------------------------------------------------------------------------
// PythonRuntimeError
// -----------------------------------------------------------------------------

/**
 * Construct an exception object where the message is populated from the
 * current Python exception state.
 * @param withTrace If true, include the full traceback in the message
 */
PythonRuntimeError::PythonRuntimeError(bool withTrace)
    : std::runtime_error(exceptionToString(withTrace)) {}
} // namespace Environment
} // namespace PythonInterface
} // namespace Mantid
