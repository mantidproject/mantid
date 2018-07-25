#include "MantidQtWidgets/MplCpp/NDArray1D.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"
#include "MantidQtWidgets/Common/PythonThreading.h"

#include "MantidKernel/WarningSuppressions.h"

// See https://docs.scipy.org/doc/numpy/reference/c-api.array.html#miscellaneous
#define PY_ARRAY_UNIQUE_SYMBOL MPLCPP_ARRAY_API
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
// clang-format off
GCC_DIAG_OFF(cast-qual)
GCC_DIAG_OFF(pedantic)
#include <numpy/arrayobject.h>
GCC_DIAG_ON(pedantic)
GCC_DIAG_ON(cast-qual)
// clang-format on

#include <algorithm>
#include <type_traits>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
// Simply struct to aid in global initialization of numpy
struct ImportArray {
  ImportArray() {
    PythonGIL gil;
    int result = _import_array();
    if (result < 0) {
      throw PythonError();
    }
  }
  ~ImportArray() {}
};
// static instance to call import_array()
// Do not remove this!!
// ImportArray _importer;

void initializeNumpy() {
  static ImportArray importer;
  (void)importer;
}
}

namespace detail {

// Numpy macro expands to code block containing a warning.
// clang-format off
GCC_DIAG_OFF(pedantic)
// clang-format on
template <typename Iterable> PyObject *copyToNDArray(const Iterable &data) {
  static_assert(std::is_same<typename Iterable::value_type, double>::value,
                "Element type must be double.");
  initializeNumpy();
  npy_intp length = static_cast<npy_intp>(data.size());
  auto ndarray = PyArray_SimpleNew(1, &length, NPY_DOUBLE);
  if (!ndarray)
    throw PythonError();
  auto emptyData =
      static_cast<double *>(PyArray_DATA((PyArrayObject *)ndarray));
  std::copy(std::begin(data), std::end(data), emptyData);
  return ndarray;
}
// clang-format off
GCC_DIAG_ON(pedantic)
// clang-format on

// Explicit template instantiations
template EXPORT_OPT_MANTIDQT_MPLCPP PyObject *
copyToNDArray<std::vector<double>>(const std::vector<double> &);
}

/**
 * Create a new reference to an array
 * @param ptr A pointer to a type of numpy.ndarray
 * @return A new wrapper around the bare pointer
 */
template <typename ElementType>
NDArray1D<ElementType> NDArray1D<ElementType>::fromNewRef(PyObject *ptr) {
  if (!ptr)
    throw PythonError();
  return NDArray1D(ptr);
}

/**
 * Access the shape of the array
 * @return A single element array with the length of the array
 */
template <typename ElementType>
std::array<size_t, 1> NDArray1D<ElementType>::shape() const {
  auto npShape = PyArray_SHAPE((PyArrayObject *)(this->get()));
  return {{static_cast<size_t>(npShape[0])}};
}

/**
 * Access an element of the array. Note that there are currently no
 * checks on the validity of the index
 * @return An element of the array
 */
template <typename ElementType>
ElementType NDArray1D<ElementType>::operator[](size_t i) const {
  return *static_cast<ElementType *>(
             PyArray_GETPTR1((PyArrayObject *)this->get(), i));
}

//
// Explicit template instantiations
//
template class EXPORT_OPT_MANTIDQT_MPLCPP NDArray1D<double>;
}
}
}
