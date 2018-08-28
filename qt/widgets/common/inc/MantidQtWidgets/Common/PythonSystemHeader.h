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
#include <frameobject.h>
#include <traceback.h>

// Macros for 2/3 compatability
#if PY_VERSION_HEX >= 0x03000000
#define IS_PY3K
#define INT_CHECK PyLong_Check
#define TO_LONG PyLong_AsLong
#define FROM_LONG PyLong_FromLong
#define STR_CHECK PyUnicode_Check
#define TO_CSTRING _PyUnicode_AsString
#define FROM_CSTRING PyUnicode_FromString
#define CODE_OBJECT(x) x
#else
#define INT_CHECK PyInt_Check
#define TO_LONG PyInt_AsLong
#define STR_CHECK PyString_Check
#define TO_CSTRING PyString_AsString
#define FROM_CSTRING PyString_FromString
#define CODE_OBJECT(x) (PyCodeObject *)x
#define FROM_LONG PyInt_FromLong
#endif

#endif // PYTHONSYSTEMHEADER_H_
