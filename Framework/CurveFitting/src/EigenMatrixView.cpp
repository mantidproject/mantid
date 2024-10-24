// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/EigenMatrixView.h"

namespace Mantid::CurveFitting {
// EigenMatrix_View Constructors
// default constructor
EigenMatrix_View::EigenMatrix_View() : m_view({}, 0, 0, dynamic_stride(0, 0)), m_isConst(false) {}

// constructor: array->matrix view
/// @param base: array from which to take view.
/// @param nTotalRows: total number of rows in the subject matrix.
/// @param nTotalCols: total number of columns in the subject matrix.
/// @param nElements_1: number of elements to include in view in dimension 1 (rows).
/// @param nElements_2: number of elements to include in view in dimension 2 (cols).
/// @param startElement_1: index number of element to start view on, dimension 1 (rows).
/// @param startElement_2: index number of element to start view on, dimension 2 (cols).
EigenMatrix_View::EigenMatrix_View(double *base, const size_t nTotalRows, size_t nTotalCols, size_t nElements_1,
                                   size_t nElements_2, const size_t startElement_1, const size_t startElement_2)
    : m_view(base, nTotalRows, nTotalCols, dynamic_stride(nTotalRows, 1)) {
  if (nElements_1 == SIZE_T_NULL && nElements_2 == SIZE_T_NULL)
    // if both are default, exit as m_view is initialised as such
    return;
  initialiseMatrix(nTotalRows, nTotalCols, nElements_1, nElements_2);
  new (&m_view) map_type(base + (startElement_2 * nTotalRows) + startElement_1, nElements_1, nElements_2,
                         dynamic_stride(nTotalRows, 1));
}

// constructor: matrix->matrix view
/// @param matrix: Eigen::MatrixXd from which to take view.
/// @param nElements_1: number of elements to include in view in dimension 1 (rows).
/// @param nElements_2: number of elements to include in view in dimension 2 (cols).
/// @param startElement_1: index number of element to start view on, dimension 1 (rows).
/// @param startElement_2: index number of element to start view on, dimension 2 (cols).
EigenMatrix_View::EigenMatrix_View(Eigen::MatrixXd &matrix, size_t nElements_1, size_t nElements_2,
                                   const size_t startElement_1, const size_t startElement_2)
    : m_view(matrix.data(), matrix.rows(), matrix.cols(), dynamic_stride(matrix.outerStride(), matrix.innerStride())) {
  if (nElements_1 == SIZE_T_NULL && nElements_2 == SIZE_T_NULL)
    // if both are default, exit as m_view is initialised as such
    return;
  initialiseMatrix(matrix.rows(), matrix.cols(), nElements_1, nElements_2);
  new (&m_view) map_type(matrix.data() + (startElement_2 * matrix.rows()) + startElement_1, nElements_1, nElements_2,
                         dynamic_stride(matrix.outerStride(), matrix.innerStride()));
}

// constructor: map->matrix view
/// @param matrix:  Eigen::Map of an Eigen::MatrixXd from which to take view.
/// @param nElements_1: number of elements to include in view in dimension 1 (rows).
/// @param nElements_2: number of elements to include in view in dimension 2 (cols).
/// @param startElement_1: index number of element to start view on, dimension 1 (rows).
/// @param startElement_2: index number of element to start view on, dimension 2 (cols).
EigenMatrix_View::EigenMatrix_View(map_type &matrix, size_t nElements_1, size_t nElements_2,
                                   const size_t startElement_1, const size_t startElement_2)
    : m_view(matrix.data(), matrix.rows(), matrix.cols(), dynamic_stride(matrix.outerStride(), matrix.innerStride())) {
  if (nElements_1 == SIZE_T_NULL && nElements_2 == SIZE_T_NULL)
    // if both are default, exit as m_view is initialised as such
    return;
  initialiseMatrix(matrix.rows(), matrix.cols(), nElements_1, nElements_2);
  new (&m_view) map_type(matrix.data() + (startElement_2 * matrix.rows()) + startElement_1, nElements_1, nElements_2,
                         dynamic_stride(matrix.outerStride(), matrix.innerStride()));
}

/// CONST constructor: array->matrix view
/// @param base: array from which to take view.
/// @param nTotalRows: total number of rows in the subject matrix.
/// @param nTotalCols: total number of columns in the subject matrix.
/// @param nElements_1: number of elements to include in view in dimension 1 (rows).
/// @param nElements_2: number of elements to include in view in dimension 2 (cols).
/// @param startElement_1: index number of element to start view on, dimension 1 (rows).
/// @param startElement_2: index number of element to start view on, dimension 2 (cols).
EigenMatrix_View::EigenMatrix_View(const double *base, const size_t nTotalRows, size_t nTotalCols, size_t nElements_1,
                                   size_t nElements_2, const size_t startElement_1, const size_t startElement_2)
    : m_view({}, 0, 0, dynamic_stride(0, 0)), m_isConst(true) {
  initialiseMatrix(nTotalRows, nTotalCols, nElements_1, nElements_2);
  new (&m_view) const_map_type(base + (startElement_2 * nTotalRows) + startElement_1, nElements_1, nElements_2,
                               dynamic_stride(nTotalRows, 1));
}

/// CONST constructor: matrix->matrix view
/// @param matrix: Eigen::MatrixXd from which to take view.
/// @param nElements_1: number of elements to include in view in dimension 1 (rows).
/// @param nElements_2: number of elements to include in view in dimension 2 (cols).
/// @param startElement_1: index number of element to start view on, dimension 1 (rows).
/// @param startElement_2: index number of element to start view on, dimension 2 (cols).
EigenMatrix_View::EigenMatrix_View(const Eigen::MatrixXd &matrix, size_t nElements_1, size_t nElements_2,
                                   const size_t startElement_1, const size_t startElement_2)
    : m_view({}, 0, 0, dynamic_stride(0, 0)), m_isConst(true) {
  initialiseMatrix(matrix.rows(), matrix.cols(), nElements_1, nElements_2);
  new (&m_view) const_map_type(matrix.data() + (startElement_2 * matrix.rows()) + startElement_1, nElements_1,
                               nElements_2, dynamic_stride(matrix.rows(), 1));
}

/// CONST constructor: map->matrix view
/// @param matrix:  Eigen::Map of an Eigen::MatrixXd from which to take view.
/// @param nElements_1: number of elements to include in view in dimension 1 (rows).
/// @param nElements_2: number of elements to include in view in dimension 2 (cols).
/// @param startElement_1: index number of element to start view on, dimension 1 (rows).
/// @param startElement_2: index number of element to start view on, dimension 2 (cols).
EigenMatrix_View::EigenMatrix_View(const map_type &matrix, size_t nElements_1, size_t nElements_2,
                                   const size_t startElement_1, const size_t startElement_2)
    : m_view({}, 0, 0, dynamic_stride(0, 0)), m_isConst(true) {
  initialiseMatrix(matrix.rows(), matrix.cols(), nElements_1, nElements_2);
  new (&m_view) const_map_type(matrix.data() + (startElement_2 * matrix.rows()) + startElement_1, nElements_1,
                               nElements_2, dynamic_stride(matrix.outerStride(), matrix.innerStride()));
}

/// CONST copy constructor
/// @param v :: EigenMatrix_View to copy.
EigenMatrix_View::EigenMatrix_View(const EigenMatrix_View &v)
    : m_view({}, 0, 0, dynamic_stride(0, 0)), m_isConst(true) {
  new (&m_view)
      const_map_type(v.matrix_inspector().data(), v.rows(), v.cols(), dynamic_stride(v.outerStride(), v.innerStride()));
}

/// copy constructor
/// @param v :: EigenMatrix_View to copy.
EigenMatrix_View::EigenMatrix_View(EigenMatrix_View &v)
    : m_view(v.matrix_mutator().data(), v.rows(), v.cols(), dynamic_stride(v.outerStride(), v.innerStride())),
      m_isConst(false) {}

/// @returns a non-const reference to the member m_view, an Eigen::Map of an Eigen::MatrixXd.
map_type &EigenMatrix_View::matrix_mutator() {
  if (!m_isConst) {
    return m_view;
  } else {
    throw std::runtime_error("Matrix is const matrix, a const matrix cannot be mutated.");
  }
}

EigenMatrix_View &EigenMatrix_View::operator=(EigenMatrix_View &V) {
  m_isConst = V.m_isConst;
  new (&m_view) map_type(V.m_view.data(), V.m_view.rows(), V.m_view.cols(),
                         dynamic_stride(V.m_view.outerStride(), V.m_view.innerStride()));
  return *this;
}

EigenMatrix_View &EigenMatrix_View::operator=(EigenMatrix_View &&V) {
  m_isConst = V.m_isConst;
  new (&m_view) map_type(V.m_view.data(), V.m_view.rows(), V.m_view.cols(),
                         dynamic_stride(V.m_view.outerStride(), V.m_view.innerStride()));
  return *this;
}
} // namespace Mantid::CurveFitting
