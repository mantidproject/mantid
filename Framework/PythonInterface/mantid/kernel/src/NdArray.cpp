#include "MantidPythonInterface/kernel/NdArray.h"
#include "MantidPythonInterface/kernel/Converters/PyArrayType.h"

#include <boost/python/detail/prefix.hpp> // Safe include of Python.h
#include <boost/python/tuple.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using Mantid::PythonInterface::Converters::getNDArrayType;
using namespace boost::python;

namespace Mantid {
namespace PythonInterface {
namespace NumPy {

namespace {
inline PyArrayObject *rawArray(const NdArray &obj) {
  return (PyArrayObject *)obj.ptr();
}
} // namespace

// -----------------------------------------------------------------------------
// NdArray - public methods
// -----------------------------------------------------------------------------

/**
 * Check if a python object points to an array type object
 * @param obj A pointer to an arbitrary python object
 * @returns True if the underlying object is an ndarray, false otherwise
 */
bool NdArray::check(const object &obj) { return PyArray_Check(obj.ptr()); }

/**
 * Construction from a plain object. Assumes the array is actually a
 * a numpy array
 * @param obj A wrapper around a Python object pointing to a numpy array
 */
NdArray::NdArray(const object &obj)
    : object(detail::borrowed_reference(obj.ptr())) {}

/**
 * @return Return the shape of the array
 */
Py_intptr_t const *NdArray::get_shape() const {
  return PyArray_DIMS(rawArray(*this));
}

/**
 * @return Return the number of dimensions of the array
 */
int NdArray::get_nd() const { return PyArray_NDIM(rawArray(*this)); }

/**
 * This returns char so stride math works properly on it. It's pretty much
 * expected that the user will have to reinterpret_cast it.
 * @return The array's raw data pointer
 */
void *NdArray::get_data() const { return PyArray_DATA(rawArray(*this)); }

/**
 * Casts (and copies if necessary) the array to the given data type
 * @param dtype Character code for the numpy data types
 * (https://docs.scipy.org/doc/numpy/reference/arrays.dtypes.html)
 * @param copy If true then the return array is always a copy otherwise
 * the returned array will only be copied if necessary
 * @return A numpy array with values of the requested type
 */
NdArray NdArray::astype(char dtype, bool copy) const {
  auto callable = object(handle<>(
      PyObject_GetAttrString(this->ptr(), const_cast<char *>("astype"))));
  auto args = tuple();
  auto kwargs = object(handle<>(Py_BuildValue(
      const_cast<char *>("{s:c,s:i}"), "dtype", dtype, "copy", copy ? 1 : 0)));
  return NdArray(boost::python::detail::new_reference(
      PyObject_Call(callable.ptr(), args.ptr(), kwargs.ptr())));
}
} // namespace NumPy
} // namespace PythonInterface
} // namespace Mantid

// -----------------------------------------------------------------------------
// object_manager_traits specialisation for NdArray
// -----------------------------------------------------------------------------
namespace boost {
namespace python {
namespace converter {

/**
 * Check if the given object is an instance of the array type
 * @param obj A python object instance
 * @return True if the type matches numpy.ndarray
 */
bool object_manager_traits<Mantid::PythonInterface::NumPy::NdArray>::check(
    PyObject *obj) {
  return ::PyObject_IsInstance(obj, (PyObject *)getNDArrayType());
}

/**
 * Create a boost::python object handle from the raw PyObject if it is
 * a matching type.
 * @param obj A python object instance
 * @return A new_reference holder wrapped around the raw Python object
 * or a nullptr if the types don't match
 */
python::detail::new_reference
object_manager_traits<Mantid::PythonInterface::NumPy::NdArray>::adopt(
    PyObject *obj) {
  return python::detail::new_reference(
      python::pytype_check(getNDArrayType(), obj));
}

/**
 * Return the PyTypeObject for this type
 * @return A pointer to the PyTypeObject defining the Python type
 */
PyTypeObject const *
object_manager_traits<Mantid::PythonInterface::NumPy::NdArray>::get_pytype() {
  return getNDArrayType();
}
} // namespace converter
} // namespace python
} // namespace boost
