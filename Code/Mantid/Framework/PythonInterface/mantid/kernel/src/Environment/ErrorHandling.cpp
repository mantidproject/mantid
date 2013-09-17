//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidPythonInterface/kernel/Environment/ErrorHandling.h"
#include "MantidPythonInterface/kernel/Environment/Threading.h"

#include <frameobject.h> //Python

#include <sstream>
#include <stdexcept>

namespace Mantid
{
namespace PythonInterface
{
namespace Environment
{
  namespace 
  {
    void tracebackToMsg(std::stringstream & msg, PyTracebackObject* traceback, bool root=true) 
    {
      if(traceback == NULL) return;
      msg << "\n  ";
      if (root)
        msg << "at";
      else
        msg << "caused by";
      
      msg << " line " << traceback->tb_lineno
          << " in \'" << PyString_AsString(traceback->tb_frame->f_code->co_filename) << "\'";
      tracebackToMsg(msg, traceback->tb_next, false);
    }
  }

  /**
   * Convert a python error state to a C++ exception
   * @param withTrace If true then a traceback will be included in the exception message
   * @throws std::runtime_error
   */
  void throwRuntimeError(const bool withTrace)
  {
    GlobalInterpreterLock gil;
    if( !PyErr_Occurred() ) 
    {
      throw std::runtime_error("ErrorHandling::throwRuntimeError - No Python error state set!");
    }
    PyObject *exception(NULL), *value(NULL), *traceback(NULL);
    PyErr_Fetch(&exception, &value, &traceback);
    PyErr_NormalizeException(&exception, &value, &traceback);
    PyErr_Clear();
    PyObject *str_repr = PyObject_Str(value);
    std::stringstream msg;
    if( value && str_repr )
    {
      msg << PyString_AsString(str_repr);
    }
    else
    {
      msg << "Unknown exception has occurred.";
    }
    if (withTrace)
    {
      tracebackToMsg(msg, (PyTracebackObject *)(traceback));
    }

    // Ensure we decrement the reference count on the traceback and exception 
    // objects as they hold references to local the stack variables that were
    // present when the exception was raised. This could include child algorithms
    // with workspaces stored that would not otherwise be cleaned up until the
    // program exited.
    Py_XDECREF(traceback);
    Py_XDECREF(exception);
    Py_XDECREF(value);
    // Raise this error as a C++ error
    throw std::runtime_error(msg.str());
  }

}
}
}
