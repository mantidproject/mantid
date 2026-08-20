// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/api/CloneMatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>

#include <stdexcept>

// See
// http://docs.scipy.org/doc/numpy/reference/c-api.array.html#PY_ARRAY_UNIQUE_SYMBOL
#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid::PythonInterface {
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;

// ----------------------------------------------------------------------------------------------------------
namespace {
/// Which data field are we extracting
enum DataField { XValues = 0, YValues = 1, EValues = 2, DxValues = 3 };

/**
 * Read-only access to the values one spectrum holds for the field being extracted.
 * @tparam Field :: Which field should be extracted
 * @param workspace :: The workspace that contains the data
 * @param index :: The workspace index to read
 * @return A const reference to the histogram data of that field
 */
template <DataField Field> decltype(auto) fieldData(MatrixWorkspace const &workspace, size_t const index) {
  throw std::logic_error("fieldData does not handle this DataField");
}

template <> decltype(auto) fieldData<XValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.x(index);
}

template <> decltype(auto) fieldData<YValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.y(index);
}

template <> decltype(auto) fieldData<EValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.e(index);
}

template <> decltype(auto) fieldData<DxValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.dx(index);
}

/**
 * The number of values one spectrum holds for the field being extracted
 * @tparam Field :: Which field should be extracted
 * @param workspace :: The workspace that contains the data
 * @param index :: The workspace index to read
 * @return The number of values the field holds for that spectrum
 */
template <DataField Field> size_t fieldSize(MatrixWorkspace const &workspace, size_t const index) {
  throw std::logic_error("fieldSize does not handle this DataField");
}

template <> size_t fieldSize<XValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.x(index).size();
}

template <> size_t fieldSize<YValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.histogramSize(index);
}

template <> size_t fieldSize<EValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.histogramSize(index);
}

template <> size_t fieldSize<DxValues>(MatrixWorkspace const &workspace, size_t const index) {
  return workspace.histogramSize(index);
}

/**
 * Helper method for extraction to numpy.
 * @tparam Field :: Which field should be extracted
 * @param workspace :: A pointer to the workspace that contains the data
 * @param start :: The index in the workspace to start at when reading the data
 * @param endp1 :: One past the end index in the workspace to finish at when
 *reading the data (similar to .end() for STL)
 *
 */
template <DataField Field>
PyArrayObject *cloneArray(MatrixWorkspace const &workspace, size_t const start, size_t const endp1) {
  npy_intp const numHist(endp1 - start);
  npy_intp const stride = numHist > 0 ? static_cast<npy_intp>(fieldSize<Field>(workspace, start)) : 0;

  // For numpy 2D array ensure all spectra have same number of values
  for (npy_intp i = 1; i < numHist; ++i) {
    if (static_cast<npy_intp>(fieldSize<Field>(workspace, start + i)) != stride) {
      throw std::length_error("Cannot extract data from a ragged workspace: the histograms do not all have the same "
                              "number of values.");
    }
  }

  npy_intp arrayDims[2] = {numHist, stride};
  auto *nparray =
      reinterpret_cast<PyArrayObject *>(PyArray_NewFromDescr(&PyArray_Type, PyArray_DescrFromType(NPY_DOUBLE),
                                                             2,         // rank 2
                                                             arrayDims, // Length in each dimension
                                                             nullptr, nullptr, 0, nullptr));
  // prevent segfault by ensuring that the array was created successfully
  // otherwise, numpy has set the error indicator already, e.g. MemoryError for a workspace too large to fit
  if (nparray == nullptr) {
    throw boost::python::error_already_set();
  }
  auto *dest = reinterpret_cast<double *>(PyArray_DATA(nparray)); // HEAD of the contiguous numpy data array

  PARALLEL_FOR_IF(threadSafe(workspace))
  for (npy_intp i = 0; i < numHist; ++i) {
    auto const &src = fieldData<Field>(workspace, start + i);
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
PyObject *cloneX(const MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(cloneArray<XValues>(self, 0, self.getNumberHistograms()));
}
/* Create a numpy array from the Y values of the given workspace reference
 * This acts like a python method on a Matrixworkspace object
 * @param self :: A pointer to a PyObject representing the calling object
 * @return A 2D numpy array created from the Y values
 */
PyObject *cloneY(const MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(cloneArray<YValues>(self, 0, self.getNumberHistograms()));
}

/* Create a numpy array from the E values of the given workspace reference
 * This acts like a python method on a Matrixworkspace object
 * @param self :: A pointer to a PyObject representing the calling object
 * @return A 2D numpy array created from the E values
 */
PyObject *cloneE(const MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(cloneArray<EValues>(self, 0, self.getNumberHistograms()));
}

/* Create a numpy array from the E values of the given workspace reference
 * This acts like a python method on a Matrixworkspace object
 * @param self :: A pointer to a PyObject representing the calling object
 * @return A 2D numpy array created from the E values
 */
PyObject *cloneDx(const MatrixWorkspace &self) {
  return reinterpret_cast<PyObject *>(cloneArray<DxValues>(self, 0, self.getNumberHistograms()));
}
} // namespace Mantid::PythonInterface
