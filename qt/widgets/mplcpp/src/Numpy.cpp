#include "MantidQtWidgets/MplCpp/Numpy.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"

#include "MantidQtWidgets/Common/PythonThreading.h"

// See https://docs.scipy.org/doc/numpy/reference/c-api.array.html#miscellaneous
#define PY_ARRAY_UNIQUE_SYMBOL MPLCPP_ARRAY_API
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

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

template <typename Iterable> PythonObject copyToNDArray(const Iterable &data) {
  static_assert(std::is_same<typename Iterable::value_type, double>::value,
                "Element type must be double.");
  initializeNumpy();
  npy_intp length = static_cast<npy_intp>(data.size());
  auto dt = PyArray_DescrFromType(NPY_DOUBLE);
  Py_INCREF(dt);
  auto ndarray = PyArray_SimpleNewFromDescr(1, &length, dt);
  if (!ndarray)
    throw PythonError();
  auto emptyData =
      static_cast<double *>(PyArray_DATA((PyArrayObject *)ndarray));
  std::copy(std::begin(data), std::end(data), emptyData);
  return PythonObject(NewRef(ndarray));
}

// Explicit instantiations
template EXPORT_OPT_MANTIDQT_MPLCPP PythonObject
copyToNDArray<std::vector<double>>(const std::vector<double> &);
}
}
}
