//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/kernel/Converters/NDArrayTypeIndex.h"

#include <boost/python/list.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <string>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
namespace Impl {
/**
 * Defines the wrapWithNDArray specialization for vector container types
 *
 * Wraps a vector in a numpy array structure without copying the data
 * @param cdata :: A reference to the std::vector to wrap
 * @param mode :: A mode switch to define whether the final array is read
 *only/read-write
 * @return A pointer to a numpy ndarray object
 */
template <typename ContainerType>
PyObject *wrapWithNDArray(const ContainerType &cdata,
                          const NumpyWrapMode mode) {
  npy_intp dims[1] = {cdata.size()};
  int datatype = NDArrayTypeIndex<typename ContainerType::value_type>::typenum;
  PyObject *nparray =
      PyArray_SimpleNewFromData(1, dims, datatype, (void *)&(cdata[0]));
  if (mode == ReadOnly) {
    PyArrayObject *np = (PyArrayObject *)nparray;
    np->flags &= ~NPY_WRITEABLE;
  }
  return nparray;
}

/**
 * Returns a new numpy array with the a copy of the data from cvector. A
 * specialization
 * exists for strings so that they simply create a standard python list.
 * @param cdata :: A reference to a std::vector
 * @return
 */
template <typename ContainerType>
PyObject *cloneToNDArray(const ContainerType &cdata) {
  npy_intp dims[1] = {cdata.size()};
  int datatype = NDArrayTypeIndex<typename ContainerType::value_type>::typenum;
  PyObject *nparray =
      PyArray_NewFromDescr(&PyArray_Type, PyArray_DescrFromType(datatype),
                           1,    // rank 1
                           dims, // Length in each dimension
                           NULL, NULL, 0, NULL);

  void *arrayData = PyArray_DATA(nparray);
  const void *data = cdata.data();
  std::memcpy(arrayData, data, PyArray_ITEMSIZE(nparray) * dims[0]);
  return (PyObject *)nparray;
}

/**
 * Returns a new python list of strings from the given vector
 * exists for strings so that they simply create a standard python list.
 * @param cdata :: A reference to a std::vector
 * @return
 */
template <> PyObject *cloneToNDArray(const std::vector<std::string> &cdata) {
  boost::python::list pystrs;
  for (auto iter = cdata.begin(); iter != cdata.end(); ++iter) {
    pystrs.append(iter->c_str());
  }
  PyObject *rawptr = pystrs.ptr();
  Py_INCREF(rawptr); // Make sure it survies after the wrapper decrefs the count
  return rawptr;
}

//-----------------------------------------------------------------------
// Explicit instantiations
//-----------------------------------------------------------------------
#define INSTANTIATE_TONDARRAY(ElementType)                                     \
  template DLLExport PyObject *wrapWithNDArray<std::vector<ElementType>>(      \
      const std::vector<ElementType> &, const NumpyWrapMode);                  \
  template DLLExport PyObject *cloneToNDArray<std::vector<ElementType>>(       \
      const std::vector<ElementType> &);

///@cond Doxygen doesn't seem to like this...
INSTANTIATE_TONDARRAY(int);
INSTANTIATE_TONDARRAY(long);
INSTANTIATE_TONDARRAY(long long);
INSTANTIATE_TONDARRAY(unsigned int);
INSTANTIATE_TONDARRAY(unsigned long);
INSTANTIATE_TONDARRAY(unsigned long long);
INSTANTIATE_TONDARRAY(double);
///@endcond
// std::string already has clone instantiated as it is specialized
template DLLExport PyObject *
wrapWithNDArray<std::vector<std::string>>(const std::vector<std::string> &,
                                          const NumpyWrapMode);
} // namespace Impl
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
