#ifndef PYTHONSYSTEMHEADER_H_
#define PYTHONSYSTEMHEADER_H_

//  This file serves as a wrapper around <Python.h> which allows it to be
//  compiled with GCC 2.95.2 under Win32 and which disables the default MSVC
//  behavior so that a program may be compiled in debug mode without requiring a
//  special debugging build of the Python library.
#include <boost/python/detail/wrap_python.hpp>

// A few more Python headers
#include <compile.h>
#include <eval.h>
#include <traceback.h>
#include <frameobject.h>

#endif // PYTHONSYSTEMHEADER_H_
