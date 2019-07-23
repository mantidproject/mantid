// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/core/NDArray.h"

#include <boost/python/extract.hpp>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

using boost::python::extract;

namespace Mantid {
namespace PythonInterface {

namespace Converters {
/**
 * Construct the converter object with the given Python object
 * @param p :: A boost::python object is either a wrapped Kernel::Matrix
 * or 2D numpy array
 * Throws std::invalid_argument if not
 * if that is not the case.
 */
PyObjectToMatrix::PyObjectToMatrix(const boost::python::object &p)
    : m_obj(p), m_alreadyMatrix(false) {
  // Is it an already wrapped V3D ?
  extract<Kernel::Matrix<double>> converter(p);
  if (converter.check()) {
    m_alreadyMatrix = true;
    return;
  }
  // Is it a 2D numpy array
  if (!NDArray::check(p)) {
    std::ostringstream msg;
    msg << "Cannot convert object to Matrix. Expected numpy array, found "
        << p.ptr()->ob_type->tp_name;
    throw std::invalid_argument(msg.str());
  }
  const auto ndim = PyArray_NDIM((PyArrayObject *)p.ptr());
  if (ndim != 2) {
    std::ostringstream msg;
    msg << "Error converting numpy array to Matrix. Expected ndim=2, found "
           "ndim="
        << ndim << " dimensions.";
    throw std::invalid_argument(msg.str());
  }
}

/**
 * Returns a V3D object from the Python object given
 * to the converter
 * @returns A newly constructed V3D object converted
 * from the PyObject.
 */
Kernel::Matrix<double> PyObjectToMatrix::operator()() {
  if (m_alreadyMatrix) {
    return extract<Kernel::Matrix<double>>(m_obj)();
  }
  auto *ndarray = (PyArrayObject *)PyArray_View(
      (PyArrayObject *)m_obj.ptr(), PyArray_DescrFromType(NPY_DOUBLE),
      &PyArray_Type);
  const auto shape = PyArray_DIMS(ndarray);
  npy_intp nx(shape[0]), ny(shape[1]);
  Kernel::Matrix<double> matrix(nx, ny);
  for (npy_intp i = 0; i < nx; i++) {
    auto row = matrix[i];
    for (npy_intp j = 0; j < ny; j++) {
      row[j] = *((double *)PyArray_GETPTR2(ndarray, i, j));
    }
  }
  return matrix;
}
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
