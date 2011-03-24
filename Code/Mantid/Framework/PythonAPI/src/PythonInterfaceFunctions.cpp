//-------------------------------------------
// Includes
//-------------------------------------------
#include <sstream>
#include "MantidPythonAPI/PythonInterfaceFunctions.h"
#include <frameobject.h>

using std::stringstream;

namespace Mantid
{
namespace PythonAPI
{

namespace {
  void tracebackToMsg(stringstream & msg, PyTracebackObject* traceback, bool root=true) {
    if (traceback == NULL)
      return;

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

/// Convert a python error state to a C++ exception so that Mantid can catch it
void handlePythonError(const bool with_trace)
{
  if( !PyErr_Occurred() ) return;
  PyObject *exception(NULL), *value(NULL), *traceback(NULL);
  PyErr_Fetch(&exception, &value, &traceback);
  PyErr_NormalizeException(&exception, &value, &traceback);
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
  if (with_trace)
    tracebackToMsg(msg, (PyTracebackObject *)(traceback));

  throw std::runtime_error(msg.str());
}

}
}
