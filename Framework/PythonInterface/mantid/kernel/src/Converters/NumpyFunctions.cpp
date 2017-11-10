#include "MantidPythonInterface/kernel/Converters/NumpyFunctions.h"

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
}
}
}
}
