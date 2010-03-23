#ifndef PYTHONSYSTEMHEADER_H_
#define PYTHONSYSTEMHEADER_H_

// The pythonX.X/object.h file for versions <2.5 uses the 'slots' word that gets redefined in
// Qt. Need to code around this
#include <patchlevel.h> //Contains Python version number

#if PY_VERSION_HEX < 0x02050000
 #undef slots
 #include <Python.h>
 #define slots

 typedef int Py_ssize_t;
 #if PY_VERSION_HEX < 0x020400A1 
  // Also need to typedef this for early 2.4 relases
  typedef struct _traceback 
  {
    PyObject_HEAD
    struct _traceback *tb_next;
    PyFrameObject *tb_frame;
    int tb_lasti;
    int tb_lineno;
  } PyTracebackObject;
 #endif
#else
 #include <Python.h>
#endif

// A few more Python headers
#include <compile.h>
#include <eval.h>
#include <traceback.h>
#include <frameobject.h>

#endif //PYTHONSYSTEMHEADER_H_
