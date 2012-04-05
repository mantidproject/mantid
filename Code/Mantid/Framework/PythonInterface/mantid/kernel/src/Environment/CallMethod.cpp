//-------------------------------------------
// Includes
//-------------------------------------------
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"
#include <sstream>
#include <frameobject.h>

using std::stringstream;

namespace Mantid
{
namespace PythonInterface
{
namespace Environment
{
  namespace 
  {
    void tracebackToMsg(stringstream & msg, PyTracebackObject* traceback, bool root=true) 
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
   */
  void translateErrorToException(const bool withTrace)
  {
    GlobalInterpreterLock gil;
    if( !PyErr_Occurred() ) 
    {
      boost::python::throw_error_already_set();
      return;
    }
    PyObject *exception(NULL), *value(NULL), *traceback(NULL);
    PyErr_Fetch(&exception, &value, &traceback);
    PyErr_NormalizeException(&exception, &value, &traceback);
    PyErr_Clear();
    PyObject *str_repr = PyObject_Str(value);
    stringstream msg;
    msg << "Python error: ";
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
