// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/Converters/NDArrayTypeIndex.h"

#include <boost/python/list.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL CORE_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <string>

namespace {

/**
  Destructor for a capsule object being passed to Python. This helps release
  memory of arrays passed to Python.
  See: https://stackoverflow.com/questions/52731884/pyarray-simplenewfromdata
*/
template <typename T> void capsule_cleanup(PyObject *capsule) {
  auto *memory = static_cast<T *>(PyCapsule_GetPointer(capsule, NULL));
  delete[] memory;
}

} // namespace

namespace Mantid::PythonInterface::Converters {
#ifdef __APPLE__
extern template int NDArrayTypeIndex<bool>::typenum;
extern template int NDArrayTypeIndex<int>::typenum;
extern template int NDArrayTypeIndex<long>::typenum;
extern template int NDArrayTypeIndex<long long>::typenum;
extern template int NDArrayTypeIndex<unsigned int>::typenum;
extern template int NDArrayTypeIndex<unsigned long>::typenum;
extern template int NDArrayTypeIndex<unsigned long long>::typenum;
extern template int NDArrayTypeIndex<float>::typenum;
extern template int NDArrayTypeIndex<double>::typenum;
#endif

namespace {
/**
 * Flip the writable flag to ensure the array is read only
 * Numpy v1.7 removed access to the flags fields directly
 * and introduced the PyClear_Flags function.
 * @param arr A pointer to a numpy array
 */
void markReadOnly(PyArrayObject *arr) {
#if NPY_API_VERSION >= 0x00000007 //(1.7)
  PyArray_CLEARFLAGS(arr, NPY_ARRAY_WRITEABLE);
#else
  arr->flags &= ~NPY_WRITEABLE;
#endif
}
} // namespace

namespace Impl {

/**
 * Defines the wrapWithNDArray specialization for C array types
 *
 * Wraps an array in a numpy array structure without copying the data
 * @param carray :: A pointer to the HEAD of the array
 * @param ndims :: The dimensionality of the array
 * @param dims :: The length of the arrays in each dimension
 * @param mode :: A mode switch to define whether the final array is read
 *only/read-write
 * @param oMode :: A mode switch defining whether to transfer ownership of the
 *returned array to python
 * @return A pointer to a numpy ndarray object
 */
template <typename ElementType>
PyObject *wrapWithNDArray(const ElementType *carray, const int ndims, Py_intptr_t *dims, const NumpyWrapMode mode,
                          const OwnershipMode oMode /* = Cpp */) {
  int datatype = NDArrayTypeIndex<ElementType>::typenum;
  auto *nparray = (PyArrayObject *)PyArray_SimpleNewFromData(ndims, dims, datatype,
                                                             static_cast<void *>(const_cast<ElementType *>(carray)));

  if (oMode == Python) {
    PyObject *capsule = PyCapsule_New(const_cast<ElementType *>(carray), NULL, capsule_cleanup<ElementType>);
    PyArray_SetBaseObject((PyArrayObject *)nparray, capsule);
  }

  if (mode == ReadOnly)
    markReadOnly(nparray);
  return reinterpret_cast<PyObject *>(nparray);
}

//-----------------------------------------------------------------------
// Explicit instantiations
//-----------------------------------------------------------------------
#define INSTANTIATE_WRAPNUMPY(ElementType)                                                                             \
  template DLLExport PyObject *wrapWithNDArray<ElementType>(const ElementType *, const int ndims, Py_intptr_t *dims,   \
                                                            const NumpyWrapMode mode, const OwnershipMode oMode);

///@cond Doxygen doesn't seem to like this...
INSTANTIATE_WRAPNUMPY(int)
INSTANTIATE_WRAPNUMPY(long)
INSTANTIATE_WRAPNUMPY(long long)
INSTANTIATE_WRAPNUMPY(unsigned int)
INSTANTIATE_WRAPNUMPY(unsigned long)
INSTANTIATE_WRAPNUMPY(unsigned long long)
INSTANTIATE_WRAPNUMPY(double)
INSTANTIATE_WRAPNUMPY(float)
///@endcond
} // namespace Impl
} // namespace Mantid::PythonInterface::Converters
