// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_CORE_PYTHONCOMPAT_H
#define MANTID_PYTHONINTERFACE_CORE_PYTHONCOMPAT_H

#include "MantidPythonInterface/core/WrapPython.h"

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
#define IS_PY2K
#define INT_CHECK PyInt_Check
#define TO_LONG PyInt_AsLong
#define STR_CHECK PyString_Check
#define TO_CSTRING PyString_AsString
#define FROM_CSTRING PyString_FromString
#define CODE_OBJECT(x) (PyCodeObject *)x
#define FROM_LONG PyInt_FromLong
#endif

#endif // MANTID_PYTHONINTERFACE_CORE_PYTHONCOMPAT_H
