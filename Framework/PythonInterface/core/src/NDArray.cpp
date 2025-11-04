// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/NDArray.h"

#include <boost/python/detail/prefix.hpp> // Safe include of Python.h
#include <boost/python/tuple.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL CORE_ARRAY_API
#include <numpy/arrayobject.h>

using namespace boost::python;

namespace Mantid::PythonInterface {

/**
 * Initialize the numpy array api for this DLL.
 * @throws runtime_error if Python is not initialized
 */
void importNumpy() {
  if (!Py_IsInitialized()) {
    throw std::runtime_error("Library requires an active Python interpreter.\n"
                             "Call Py_Initialize at an appropriate point in the application.");
  }

  if (_import_array() < 0) {
    PyErr_Print();
    PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
  }
}

/**
 * @brief Return the type object for a numpy.NDArray
 * @return PyTypeObject* to the Python C-type of nump.NDArray
 */
PyTypeObject *ndarrayType() { return &PyArray_Type; }

// -----------------------------------------------------------------------------
// NDArray
// -----------------------------------------------------------------------------

/**
 * Check if a python object points to an array type object
 * @param obj A pointer to an arbitrary python object
 * @returns True if the underlying object is an NDArray, false otherwise
 */
bool NDArray::check(const object &obj) { return PyArray_Check(obj.ptr()); }

/**
 * Construction from a plain object. Assumes the array is actually a
 * a numpy array
 * @param obj A wrapper around a Python object pointing to a numpy array
 */
NDArray::NDArray(const object &obj) : object(detail::borrowed_reference(obj.ptr())) {}

/**
 * @return Return the shape of the array
 */
Py_intptr_t const *NDArray::get_shape() const { return PyArray_DIMS(reinterpret_cast<PyArrayObject *>(this->ptr())); }

/**
 * @return Return the number of dimensions of the array
 */
int NDArray::get_nd() const { return PyArray_NDIM(reinterpret_cast<PyArrayObject *>(this->ptr())); }

/**
 * This returns char so stride math works properly on it. It's pretty much
 * expected that the user will have to reinterpret_cast it.
 * @return The array's raw data pointer
 */
void *NDArray::get_data() const { return PyArray_DATA(reinterpret_cast<PyArrayObject *>(this->ptr())); }

/**
 * See https://docs.scipy.org/doc/numpy/reference/arrays.dtypes.html
 * @return The character code for the dtype of the array
 */
char NDArray::get_typecode() const { return PyArray_DESCR(reinterpret_cast<PyArrayObject *>(this->ptr()))->type; }

/**
 * Casts (and copies if necessary) the array to the given data type
 * @param dtype Character code for the numpy data types
 * (https://docs.scipy.org/doc/numpy/reference/arrays.dtypes.html)
 * @param copy If true then the return array is always a copy otherwise
 * the returned array will only be copied if necessary
 * @return A numpy array with values of the requested type
 */
NDArray NDArray::astype(char dtype, bool copy) const {
  auto callable = object(handle<>(PyObject_GetAttrString(this->ptr(), const_cast<char *>("astype"))));
  auto args = tuple();
  auto kwargs = object(handle<>(Py_BuildValue(const_cast<char *>("{s:c,s:i}"), "dtype", dtype, "copy", copy ? 1 : 0)));
  return NDArray(boost::python::detail::new_reference(PyObject_Call(callable.ptr(), args.ptr(), kwargs.ptr())));
}

} // namespace Mantid::PythonInterface

// -----------------------------------------------------------------------------
// object_manager_traits specialisation for NDArray
// -----------------------------------------------------------------------------
namespace boost::python::converter {

using Mantid::PythonInterface::ndarrayType;

/**
 * Check if the given object is an instance of the array type
 * @param obj A python object instance
 * @return True if the type matches numpy.NDArray
 */
bool object_manager_traits<Mantid::PythonInterface::NDArray>::check(PyObject *obj) {
  return ::PyObject_IsInstance(obj, reinterpret_cast<PyObject *>(ndarrayType()));
}

/**
 * Create a boost::python object handle from the raw PyObject if it is
 * a matching type.
 * @param obj A python object instance
 * @return A new_reference holder wrapped around the raw Python object
 * or a nullptr if the types don't match
 */
python::detail::new_reference object_manager_traits<Mantid::PythonInterface::NDArray>::adopt(PyObject *obj) {
  return python::detail::new_reference(python::pytype_check(ndarrayType(), obj));
}

/**
 * Return the PyTypeObject for this type
 * @return A pointer to the PyTypeObject defining the Python type
 */
PyTypeObject const *object_manager_traits<Mantid::PythonInterface::NDArray>::get_pytype() { return ndarrayType(); }
} // namespace boost::python::converter
