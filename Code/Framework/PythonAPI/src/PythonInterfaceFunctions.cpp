//-------------------------------------------
// Includes
//-------------------------------------------
#include <MantidPythonAPI/PythonInterfaceFunctions.h>

namespace Mantid
{
namespace PythonAPI
{

/// Convert a python error state to a C++ exception so that Mantid can catch it
void handlePythonError()
{
  if( !PyErr_Occurred() ) return;
  PyObject *exception(NULL), *value(NULL), *traceback(NULL);
  PyErr_Fetch(&exception, &value, &traceback);
  PyErr_NormalizeException(&exception, &value, &traceback);
  PyObject *str_repr = PyObject_Str(value);
  std::string msg("Python error: ");
  if( value && str_repr )
  {
    msg += std::string(PyString_AsString(str_repr));
  }
  else
  {
    msg += "Unknown exception has occurred.";
  }
  throw std::runtime_error(msg);
}

/**
 * Check if the current Python state will allow execution of Python code
 */
bool pythonIsReady()
{
  if( PyThreadState_GET() ) return true;
  else return false;
}

}
}
