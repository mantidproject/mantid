#include "MantidPythonInterface/kernel/NdArray.h"
#include "MantidPythonInterface/kernel/Converters/PyArrayType.h"

#include <boost/python/detail/prefix.hpp> // Safe include of Python.h
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using Mantid::PythonInterface::Converters::getNDArrayType;

namespace Mantid {
namespace PythonInterface {
namespace NumPy {

namespace {
inline PyArrayObject *rawArray(const NdArray &obj) {
  return (PyArrayObject *)obj.ptr();
}
}

// -----------------------------------------------------------------------------
// NdArray - public methods
// -----------------------------------------------------------------------------

/**
 * @return The shape of the array as a C-array
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
}
}
}

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
}
}
}
