// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/core/Converters/MatrixToNDArray.h"
#include "MantidPythonInterface/core/Converters/NDArrayTypeIndex.h"

#define PY_ARRAY_UNIQUE_SYMBOL CORE_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
namespace Impl {
/**
 * Defines the wrapWithNDArray specialization for Matrix container types
 *
 * Wraps a vector in a numpy array structure without copying the data
 * @param cdata :: A reference to the Matrix wrap
 * @param mode :: A mode switch to define whether the final array is read
 *only/read-write
 * @return A pointer to a numpy ndarray object
 */
template <typename ContainerType>
PyObject *wrapWithNDArray(const ContainerType &cdata,
                          const NumpyWrapMode mode) {
  std::pair<size_t, size_t> matrixDims = cdata.size();
  npy_intp dims[2] = {matrixDims.first, matrixDims.second};
  int datatype = NDArrayTypeIndex<typename ContainerType::value_type>::typenum;
  PyObject *ndarray =
      PyArray_SimpleNewFromData(2, dims, datatype, (void *)&(cdata[0][0]));
  if (mode == ReadOnly) {
    PyArrayObject *np = (PyArrayObject *)ndarray;
    np->flags &= ~NPY_WRITEABLE;
  }
  return ndarray;
}
//-----------------------------------------------------------------------
// Explicit instantiations
//-----------------------------------------------------------------------
#define INSTANTIATE_MATRIX_WRAP(ElementType)                                   \
  template DLLExport PyObject *wrapWithNDArray<Kernel::Matrix<ElementType>>(   \
      const Kernel::Matrix<ElementType> &, const NumpyWrapMode);

INSTANTIATE_MATRIX_WRAP(int);
INSTANTIATE_MATRIX_WRAP(float);
INSTANTIATE_MATRIX_WRAP(double);
} // namespace Impl
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
