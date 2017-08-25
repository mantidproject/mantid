#include "MantidQtWidgets/MplCpp/NDArray1D.h"

// See https://docs.scipy.org/doc/numpy/reference/c-api.array.html#miscellaneous
#define PY_ARRAY_UNIQUE_SYMBOL MPLCPP_ARRAY_API
#define NO_IMPORT_ARRAY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
/**
 * Access the shape of the array
 * @return A single element array with the length of the array
 */
std::array<size_t, 1> NDArray1D::shape() const {
  auto npShape = PyArray_SHAPE((PyArrayObject *)this->get());
  return {{static_cast<size_t>(npShape[0])}};
}
}
}
}
