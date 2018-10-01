//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/NDArrayToVector.h"
#include "MantidPythonInterface/core/Converters/NDArrayTypeIndex.h"
#include "MantidPythonInterface/kernel/Converters/NumpyFunctions.h"

#include <boost/python/extract.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/str.hpp>

#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using boost::python::extract;
using boost::python::handle;
using boost::python::object;
using boost::python::str;

namespace Mantid {
namespace PythonInterface {
namespace Converters {

extern template int NDArrayTypeIndex<bool>::typenum;
extern template int NDArrayTypeIndex<int>::typenum;
extern template int NDArrayTypeIndex<long>::typenum;
extern template int NDArrayTypeIndex<long long>::typenum;
extern template int NDArrayTypeIndex<unsigned int>::typenum;
extern template int NDArrayTypeIndex<unsigned long>::typenum;
extern template int NDArrayTypeIndex<unsigned long long>::typenum;
extern template int NDArrayTypeIndex<float>::typenum;
extern template int NDArrayTypeIndex<double>::typenum;
extern template char NDArrayTypeIndex<bool>::typecode;
extern template char NDArrayTypeIndex<int>::typecode;
extern template char NDArrayTypeIndex<long>::typecode;
extern template char NDArrayTypeIndex<long long>::typecode;
extern template char NDArrayTypeIndex<unsigned int>::typecode;
extern template char NDArrayTypeIndex<unsigned long>::typecode;
extern template char NDArrayTypeIndex<unsigned long long>::typecode;
extern template char NDArrayTypeIndex<double>::typecode;
} // namespace Converters

namespace {

//-------------------------------------------------------------------------
// Template helpers
//-------------------------------------------------------------------------
/**
 * Templated structure to fill a container
 * with values from a numpy array. It is assumed range pointed
 * to by the starting iterator has been preallocated to at least the
 * number of elements in the array and that the array is not emtpy
 */
template <typename DestElementType> struct CopyToImpl {
  void operator()(typename Converters::NDArrayToVector<
                      DestElementType>::TypedVectorIterator first,
                  PyArrayObject *arr) {
    // Use the iterator API to iterate through the array
    // and assign each value to the corresponding vector
    typedef union {
      DestElementType *output;
      void *input;
    } npy_union;
    npy_union data;
    object iter(handle<>(Converters::Impl::func_PyArray_IterNew(arr)));
    do {
      data.input = PyArray_ITER_DATA(iter.ptr());
      *first++ = *data.output;
      PyArray_ITER_NEXT(iter.ptr());
    } while (PyArray_ITER_NOTDONE(iter.ptr()));
  }
};

/**
 * Specialization for a std::string to convert the values directly
 * to the string representation
 */
template <> struct CopyToImpl<std::string> {
  void operator()(typename std::vector<std::string>::iterator first,
                  PyArrayObject *arr) {
    object flattened(handle<>(PyArray_Ravel(arr, NPY_CORDER)));
    const Py_ssize_t nelements = PyArray_Size(flattened.ptr());
    for (Py_ssize_t i = 0; i < nelements; ++i) {
      *first++ = extract<std::string>(str(flattened[i]))();
    }
  }
};

/**
 * Templated structure to check the numpy type against the C type
 * and convert the array if necessary. It is assumed that
 * the object points to numpy array, no checking is performed
 */
template <typename DestElementType> struct CoerceType {
  NDArray operator()(const NDArray &x) {
    return x.astype(Converters::NDArrayTypeIndex<DestElementType>::typecode,
                    false);
  }
};

/**
 * Specialized version for std::string as we don't need
 * to convert the underlying representation
 */
template <> struct CoerceType<std::string> {
  object operator()(const NDArray &x) { return x; }
};
} // namespace

namespace Converters {
//-------------------------------------------------------------------------
// NDArrayToVector definitions
//-------------------------------------------------------------------------

/**
 * Constructor
 * @param value :: A boost python object wrapping a numpy.ndarray
 */
template <typename DestElementType>
NDArrayToVector<DestElementType>::NDArrayToVector(const NDArray &value)
    : m_arr(CoerceType<DestElementType>()(value)) {}

/**
 * Creates a vector of the DestElementType from the numpy array of
 * given input type
 * @return A vector of the DestElementType created from the numpy array
 */
template <typename DestElementType>
const typename NDArrayToVector<DestElementType>::TypedVector
NDArrayToVector<DestElementType>::operator()() {
  std::vector<DestElementType> cvector(
      PyArray_SIZE((PyArrayObject *)m_arr.ptr()));
  copyTo(cvector);
  return cvector;
}

/**
 * @param dest A pre-sized container that will receive the values from the
 * array.
 * It's size is checked against the array size.
 */
template <typename DestElementType>
void NDArrayToVector<DestElementType>::copyTo(TypedVector &dest) const {
  if (PyArray_SIZE((PyArrayObject *)m_arr.ptr()) > 0) {
    throwIfSizeMismatched(dest);
    CopyToImpl<DestElementType>()(std::begin(dest),
                                  (PyArrayObject *)m_arr.ptr());
  }
}

/**
 * @param dest A reference to a vector whose capacity is checked that it can
 * contain the values from the numpy array
 * @throws std::invalid_argument if the vector is the wrong size or the numpy
 * array is not 1D
 */
template <typename DestElementType>
void NDArrayToVector<DestElementType>::throwIfSizeMismatched(
    const TypedVector &dest) const {
  if (PyArray_SIZE((PyArrayObject *)m_arr.ptr()) ==
      static_cast<ssize_t>(dest.size())) {
    return;
  } else {
    throw std::invalid_argument(
        "Invalid number of elements while copying from ndarray. "
        "ndarray=" +
        std::to_string(PyArray_SIZE((PyArrayObject *)m_arr.ptr())) +
        " destination=(" + std::to_string(dest.size()) + ",)");
  }
}

//------------------------------------------------------------------------
// Explicit instantiations
//------------------------------------------------------------------------
#define INSTANTIATE_TOVECTOR(ElementType)                                      \
  template struct DLLExport NDArrayToVector<ElementType>;

///@cond Doxygen doesn't seem to like this...
INSTANTIATE_TOVECTOR(int)
INSTANTIATE_TOVECTOR(long)
INSTANTIATE_TOVECTOR(long long)
INSTANTIATE_TOVECTOR(unsigned int)
INSTANTIATE_TOVECTOR(unsigned long)
INSTANTIATE_TOVECTOR(unsigned long long)
INSTANTIATE_TOVECTOR(double)
INSTANTIATE_TOVECTOR(bool)
INSTANTIATE_TOVECTOR(std::string)
///@endcond
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
