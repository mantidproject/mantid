// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/Converters/NumpyFunctions.h"

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL CORE_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
namespace Impl {

PyObject *func_PyArray_IterNew(PyArrayObject *arr) {
  return PyArray_IterNew(reinterpret_cast<PyObject *>(arr));
}

PyArrayObject *func_PyArray_NewFromDescr(int datatype, const int ndims,
                                         Py_intptr_t *dims) {
  return reinterpret_cast<PyArrayObject *>(PyArray_NewFromDescr(
      &PyArray_Type, PyArray_DescrFromType(datatype), ndims, // rank
      dims, // Length in each dimension
      nullptr, nullptr, 0, nullptr));
}

PyArrayObject *func_PyArray_NewFromDescr(const char *datadescr, const int ndims,
                                         Py_intptr_t *dims) {
  // convert the string description to an actual description
  PyArray_Descr *descr = func_PyArray_Descr(datadescr);

  // create the array
  PyArrayObject *nparray = reinterpret_cast<PyArrayObject *>(
      PyArray_NewFromDescr(&PyArray_Type, descr, ndims, // rank
                           dims, // Length in each dimension
                           nullptr, nullptr, 0, nullptr));

  return nparray;
}

PyArray_Descr *func_PyArray_Descr(const char *datadescr) {
  PyObject *data_type = Py_BuildValue("s", datadescr);
  PyArray_Descr *descr;
  PyArray_DescrConverter(data_type, &descr);
  Py_XDECREF(data_type); // function above incremented reference count

  return descr;
}
} // namespace Impl
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
