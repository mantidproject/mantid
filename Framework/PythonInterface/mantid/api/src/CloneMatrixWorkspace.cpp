// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/CloneMatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/python/extract.hpp>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid {
namespace PythonInterface {
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;

// ----------------------------------------------------------------------------------------------------------
namespace {
/// Which data field are we extracting
enum DataField { XValues = 0, YValues = 1, EValues = 2, DxValues = 3 };

/**
 * Helper method for extraction to numpy.
 * @param workspace :: A pointer to the workspace that contains the data
 * @param field :: Which field should be extracted
 * @param start :: The index in the workspace to start at when reading the data
 * @param endp1 :: One past the end index in the workspace to finish at when
 *reading the data (similar to .end() for STL)
 *
 */
PyArrayObject *cloneArray(MatrixWorkspace &workspace, DataField field,
                          const size_t start, const size_t endp1) {
  const npy_intp numHist(endp1 - start);
  npy_intp stride{0};

  // Find out which function we need to call to access the data
  using ArrayAccessFn =
      const MantidVec &(MatrixWorkspace::*)(const size_t) const;
  ArrayAccessFn dataAccesor;
  /**
   * Can do better than this with a templated object that knows how to access
   * the data
   */
  if (field == XValues) {
    stride = workspace.readX(0).size();
    dataAccesor = &MatrixWorkspace::readX;
  } else if (field == DxValues) {
    stride = workspace.readDx(0).size();
    dataAccesor = &MatrixWorkspace::readDx;
  } else {
    stride = workspace.blocksize();
    if (field == YValues)
      dataAccesor = &MatrixWorkspace::readY;
    else
      dataAccesor = &MatrixWorkspace::readE;
  }
  npy_intp arrayDims[2] = {numHist, stride};
  auto *nparray = reinterpret_cast<PyArrayObject *>(
      PyArray_NewFromDescr(&PyArray_Type, PyArray_DescrFromType(NPY_DOUBLE),
                           2,         // rank 2
                           arrayDims, // Length in each dimension
                           nullptr, nullptr, 0, nullptr));
  auto *dest = reinterpret_cast<double *>(
      PyArray_DATA(nparray)); // HEAD of the contiguous numpy data array

  PARALLEL_FOR_IF(threadSafe(workspace))
  for (npy_intp i = 0; i < numHist; ++i) {
    const MantidVec &src = (workspace.*(dataAccesor))(start + i);
    std::copy(src.begin(), src.end(), std::next(dest, i * stride));
  }
  return nparray;
}
} // namespace

// -------------------------------------- Cloned
// arrays---------------------------------------------------
/* Create a numpy array from the X values of the given workspace reference
 * This acts like a python method on a Matrixworkspace object
 * @param self :: A pointer to a PyObject representing the calling object
 * @return A 2D numpy array created from the X values
 */
PyObject *cloneX(MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(
      cloneArray(self, XValues, 0, self.getNumberHistograms()));
}
/* Create a numpy array from the Y values of the given workspace reference
 * This acts like a python method on a Matrixworkspace object
 * @param self :: A pointer to a PyObject representing the calling object
 * @return A 2D numpy array created from the Y values
 */
PyObject *cloneY(MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(
      cloneArray(self, YValues, 0, self.getNumberHistograms()));
}

/* Create a numpy array from the E values of the given workspace reference
 * This acts like a python method on a Matrixworkspace object
 * @param self :: A pointer to a PyObject representing the calling object
 * @return A 2D numpy array created from the E values
 */
PyObject *cloneE(MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(
      cloneArray(self, EValues, 0, self.getNumberHistograms()));
}

/* Create a numpy array from the E values of the given workspace reference
 * This acts like a python method on a Matrixworkspace object
 * @param self :: A pointer to a PyObject representing the calling object
 * @return A 2D numpy array created from the E values
 */
PyObject *cloneDx(MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(
      cloneArray(self, DxValues, 0, self.getNumberHistograms()));
}
} // namespace PythonInterface
} // namespace Mantid
