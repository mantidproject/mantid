// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include "MantidPythonInterface/core/DllConfig.h"
#include <boost/python/detail/wrap_python.hpp>

// Forward declare the numpy array types
struct tagPyArrayObject;
using PyArrayObject = tagPyArrayObject;
struct _PyArray_Descr;
using PyArray_Descr = _PyArray_Descr;

/*
 * The numpy API is a C api where pointers to functions and objects are the
 * same size.
 * This is not guaranteed by the C++ standard so macros in the numpy headers,
 * such as PyArray_IterNew produces warnings when compiled with a C++ compiler.
 *
 * The only solution appears to be wrap the calls in our own functions and
 * suppress the warnings.
 */
namespace Mantid {
namespace PythonInterface {
namespace Converters {
namespace Impl {
/// equivalent to macro PyArray_IterNew
MANTID_PYTHONINTERFACE_CORE_DLL PyObject *func_PyArray_IterNew(PyArrayObject *arr);
/// equivalent to macro PyArray_NewFromDescr
MANTID_PYTHONINTERFACE_CORE_DLL PyArrayObject *func_PyArray_NewFromDescr(int datatype, int ndims, Py_intptr_t *dims);
MANTID_PYTHONINTERFACE_CORE_DLL PyArrayObject *func_PyArray_NewFromDescr(const char *datadescr, int ndims,
                                                                         Py_intptr_t *dims);
MANTID_PYTHONINTERFACE_CORE_DLL PyArray_Descr *func_PyArray_Descr(const char *datadescr);
} // namespace Impl
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
