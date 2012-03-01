//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Environment/PyEnvironment.h"

#include <boost/python/detail/wrap_python.hpp>
#include <frameobject.h>
#include <cstring>

namespace Mantid
{
  namespace PythonInterface
  {
    namespace PyEnvironment
    {
      /**
       * Is the given function name in the call stack
       * @param name :: The name of the function call to search for
       * @param startFrame :: An optional frame to start from, if NULL then the current frame is
       * retrieved from the interpeter
       * @return True if the function name is found in the stack, false otherwise
       */
      bool isInCallStack(const char * name, PyFrameObject* startFrame)
      {
        PyFrameObject *frame = startFrame;
        if( !frame ) frame = PyEval_GetFrame(); // current frame
        if( !frame ) return false;
        bool inStack(false);
        if( strcmp(PyString_AsString(frame->f_code->co_name), name) == 0)
        {
          inStack = true;
        }
        else
        {
          while(frame->f_back)
          {
            frame = frame->f_back;
            if( strcmp(PyString_AsString(frame->f_code->co_name), name) == 0 )
            {
              inStack = true;
              break;
            }
          }
        }
        return inStack;
      }

    }
  }
}
