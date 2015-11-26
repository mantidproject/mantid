// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GSLMatrix.h"
#include <gsl/gsl_eigen.h>

namespace Mantid {
namespace CurveFitting {

/// Constructor
/// @param nx :: First dimension
/// @param ny :: Second dimension
GSLMatrix::GSLMatrix(const size_t nx, const size_t ny)
    : m_data(nx * ny), m_view(gsl_matrix_view_array(m_data.data(), nx, ny)) {}

/// Construct from an initialisation list
/// @param ilist :: Initialisation list as a list of rows:
///      {{M00, M01, M02, ...},
///       {M10, M11, M12, ...},
///             ...
///       {Mn0, Mn1, Mn2, ...}}
GSLMatrix::GSLMatrix(std::initializer_list<std::initializer_list<double>> ilist)
    : GSLMatrix(ilist.size(), ilist.begin()->size()) {
  for (auto row = ilist.begin(); row != ilist.end(); ++row) {
    if (row->size() != size2()) {
      throw std::runtime_error(
          "All rows in initializer list must have the same size.");
    }
    auto i = static_cast<size_t>(std::distance(ilist.begin(), row));
    for (auto cell = row->begin(); cell != row->end(); ++cell) {
      auto j = static_cast<size_t>(std::distance(row->begin(), cell));
      set(i, j, *cell);
    }
  }
}

/// Copy constructor
/// @param M :: The other matrix.
GSLMatrix::GSLMatrix(const GSLMatrix &M)
    : m_data(M.m_data),
      m_view(gsl_matrix_view_array(m_data.data(), M.size1(), M.size2())) {}

/// Create a submatrix. A submatrix is a copy of the parent matrix.
/// @param M :: The parent matrix.
/// @param row :: The first row in the submatrix.
/// @param col :: The first column in the submatrix.
/// @param nRows :: The number of rows in the submatrix.
/// @param nCols :: The number of columns in the submatrix.
GSLMatrix::GSLMatrix(const GSLMatrix &M, size_t row, size_t col, size_t nRows,
                     size_t nCols) {
  if (row + nRows > M.size1() || col + nCols > M.size2()) {
    throw std::runtime_error("Submatrix exceeds matrix size.");
  }
  auto view = gsl_matrix_const_submatrix(M.gsl(), row, col, nRows, nCols);
  m_data.resize(nRows * nCols);
  m_view = gsl_matrix_view_array(m_data.data(), nRows, nCols);
  gsl_matrix_memcpy(&m_view.matrix, &view.matrix);
}

/// Constructor
/// @param M :: A matrix to copy.
GSLMatrix::GSLMatrix(const Kernel::Matrix<double> &M)
    : m_data(M.getVector()),
      m_view(gsl_matrix_view_array(m_data.data(), M.numRows(), M.numCols())) {}

/// Create this matrix from a product of two other matrices
/// @param mult2 :: Matrix multiplication helper object.
GSLMatrix::GSLMatrix(const GSLMatrixMult2 &mult2) { *this = mult2; }

/// Create this matrix from a product of three other matrices
/// @param mult3 :: Matrix multiplication helper object.
GSLMatrix::GSLMatrix(const GSLMatrixMult3 &mult3) { *this = mult3; }

/// "Move" constructor
GSLMatrix::GSLMatrix(std::vector<double> &&data, size_t nx, size_t ny)
    : m_data(std::move(data)),
      m_view(gsl_matrix_view_array(m_data.data(), nx, ny)) {}

/// Copy assignment operator
GSLMatrix &GSLMatrix::operator=(const GSLMatrix &M) {
  m_data = M.m_data;
  m_view = gsl_matrix_view_array(m_data.data(), M.size1(), M.size2());
  return *this;
}

/// Get the pointer to the GSL matrix
gsl_matrix *GSLMatrix::gsl() { return &m_view.matrix; }
/// Get the const pointer to the GSL matrix
const gsl_matrix *GSLMatrix::gsl() const { return &m_view.matrix; }

/// Is matrix empty
bool GSLMatrix::isEmpty() const { return m_data.empty(); }

/// Resize the matrix
/// @param nx :: New first dimension
/// @param ny :: New second dimension
void GSLMatrix::resize(const size_t nx, const size_t ny) {
  m_data.resize(nx * ny);
  m_view = gsl_matrix_view_array(m_data.data(), nx, ny);
}

/// First size of the matrix
size_t GSLMatrix::size1() const { return m_view.matrix.size1; }

/// Second size of the matrix
size_t GSLMatrix::size2() const { return m_view.matrix.size2; }

/// set an element
/// @param i :: The row
/// @param j :: The column
/// @param value :: The new vaule
void GSLMatrix::set(size_t i, size_t j, double value) {
  if (isEmpty()) {
    throw std::out_of_range("Matrix is empty.");
  }
  if (i < m_view.matrix.size1 && j < m_view.matrix.size2)
    gsl_matrix_set(&m_view.matrix, i, j, value);
  else {
    throw std::out_of_range("GSLMatrix indices are out of range.");
  }
}

/// get an element
/// @param i :: The row
/// @param j :: The column
double GSLMatrix::get(size_t i, size_t j) const {
  if (isEmpty()) {
    throw std::out_of_range("Matrix is empty.");
  }
  if (i < m_view.matrix.size1 && j < m_view.matrix.size2)
    return gsl_matrix_get(&m_view.matrix, i, j);
  else {
    throw std::out_of_range("GSLMatrix indices are out of range.");
  }
}

/// Set this matrix to identity matrix
void GSLMatrix::identity() { gsl_matrix_set_identity(&m_view.matrix); }

/// Set all elements to zero
void GSLMatrix::zero() { gsl_matrix_set_zero(&m_view.matrix); }

/// Set the matrix to be diagonal.
/// @param d :: Values on the diagonal.
void GSLMatrix::diag(const GSLVector &d) {
  const auto n = d.size();
  resize(n, n);
  zero();
  for (size_t i = 0; i < n; ++i) {
    set(i, i, d.get(i));
  }
}

/// add a matrix to this
/// @param M :: A matrix
GSLMatrix &GSLMatrix::operator+=(const GSLMatrix &M) {
  gsl_matrix_add(&m_view.matrix, M.gsl());
  return *this;
}
/// add a constant to this matrix
/// @param d :: A number
GSLMatrix &GSLMatrix::operator+=(const double &d) {
  gsl_matrix_add_constant(&m_view.matrix, d);
  return *this;
}
/// subtract a matrix from this
/// @param M :: A matrix
GSLMatrix &GSLMatrix::operator-=(const GSLMatrix &M) {
  gsl_matrix_sub(&m_view.matrix, M.gsl());
  return *this;
}
/// multiply this matrix by a number
/// @param d :: A number
GSLMatrix &GSLMatrix::operator*=(const double &d) {
  gsl_matrix_scale(&m_view.matrix, d);
  return *this;
}

/// Matrix by vector multiplication
/// @param v :: A vector to multiply by. Must have the same size as size2().
/// @returns A vector - the result of the multiplication. Size of the returned
/// vector equals size1().
/// @throws std::invalid_argument if the input vector has a wrong size.
/// @throws std::runtime_error if the underlying GSL routine fails.
GSLVector GSLMatrix::operator*(const GSLVector &v) const {
  if (v.size() != size2()) {
    throw std::invalid_argument(
        "Matrix by vector multiplication: wrong size of vector.");
  }
  GSLVector res(size1());
  auto status =
      gsl_blas_dgemv(CblasNoTrans, 1.0, gsl(), v.gsl(), 0.0, res.gsl());
  if (status != GSL_SUCCESS) {
    std::string message = "Failed to multiply matrix by a vector.\n"
                          "Error message returned by the GSL:\n" +
                          std::string(gsl_strerror(status));
    throw std::runtime_error(message);
  }
  return res;
}

/// Assign this matrix to a product of two other matrices
/// @param mult2 :: Matrix multiplication helper object.
GSLMatrix &GSLMatrix::operator=(const GSLMatrixMult2 &mult2) {
  // sizes of the result matrix
  size_t n1 = mult2.tr1 ? mult2.m_1.size2() : mult2.m_1.size1();
  size_t n2 = mult2.tr2 ? mult2.m_2.size1() : mult2.m_2.size2();

  this->resize(n1, n2);

  CBLAS_TRANSPOSE tr1 = mult2.tr1 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr2 = mult2.tr2 ? CblasTrans : CblasNoTrans;

  // this = m_1 * m_2
  gsl_blas_dgemm(tr1, tr2, 1.0, mult2.m_1.gsl(), mult2.m_2.gsl(), 0.0, gsl());

  return *this;
}

/// Assign this matrix to a product of three other matrices
/// @param mult3 :: Matrix multiplication helper object.
GSLMatrix &GSLMatrix::operator=(const GSLMatrixMult3 &mult3) {
  // sizes of the result matrix
  size_t n1 = mult3.tr1 ? mult3.m_1.size2() : mult3.m_1.size1();
  size_t n2 = mult3.tr3 ? mult3.m_3.size1() : mult3.m_3.size2();

  this->resize(n1, n2);

  // intermediate matrix
  GSLMatrix AB(n1, mult3.m_2.size2());

  CBLAS_TRANSPOSE tr1 = mult3.tr1 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr2 = mult3.tr2 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr3 = mult3.tr3 ? CblasTrans : CblasNoTrans;

  // AB = m_1 * m_2
  gsl_blas_dgemm(tr1, tr2, 1.0, mult3.m_1.gsl(), mult3.m_2.gsl(), 0.0,
                 AB.gsl());

  // this = AB * m_3
  gsl_blas_dgemm(CblasNoTrans, tr3, 1.0, AB.gsl(), mult3.m_3.gsl(), 0.0, gsl());

  return *this;
}

/// Solve system of linear equations M*x == rhs, M is this matrix
/// This matrix is destroyed.
/// @param rhs :: The right-hand-side vector
/// @param x :: The solution vector
/// @throws std::invalid_argument if the input vectors have wrong sizes.
/// @throws std::runtime_error if the GSL fails to solve the equations.
void GSLMatrix::solve(const GSLVector &rhs, GSLVector &x) {
  if (size1() != size2()) {
    throw std::invalid_argument(
        "System of linear equations: the matrix must be square.");
  }
  size_t n = size1();
  if (rhs.size() != n) {
    throw std::invalid_argument(
        "System of linear equations: right-hand side vector has wrong size.");
  }
  x.resize(n);
  int s;
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_LU_decomp(gsl(), p, &s); // matrix is modified at this moment
  int res = gsl_linalg_LU_solve(gsl(), p, rhs.gsl(), x.gsl());
  gsl_permutation_free(p);
  if (res != GSL_SUCCESS) {
    std::string message = "Failed to solve system of linear equations.\n"
                          "Error message returned by the GSL:\n" +
                          std::string(gsl_strerror(res));
    throw std::runtime_error(message);
  }
}

/// Invert this matrix
void GSLMatrix::invert() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  size_t n = size1();
  int s;
  GSLMatrix LU(*this);
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_LU_decomp(LU.gsl(), p, &s);
  gsl_linalg_LU_invert(LU.gsl(), p, this->gsl());
  gsl_permutation_free(p);
}

/// Calculate the determinant
double GSLMatrix::det() const {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  size_t n = size1();
  int s;
  GSLMatrix LU(*this);
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_LU_decomp(LU.gsl(), p, &s);
  double res = gsl_linalg_LU_det(LU.gsl(), s);
  gsl_permutation_free(p);
  return res;
}

/// Calculate the eigensystem of a symmetric matrix
/// @param eigenValues :: Output variable that receives the eigenvalues of this
/// matrix.
/// @param eigenVectors :: Output variable that receives the eigenvectors of
/// this matrix.
void GSLMatrix::eigenSystem(GSLVector &eigenValues, GSLMatrix &eigenVectors) {
  size_t n = size1();
  if (n != size2()) {
    throw std::runtime_error("Matrix eigenSystem: the matrix must be square.");
  }
  eigenValues.resize(n);
  eigenVectors.resize(n, n);
  auto workspace = gsl_eigen_symmv_alloc(n);
  gsl_eigen_symmv(gsl(), eigenValues.gsl(), eigenVectors.gsl(), workspace);
  gsl_eigen_symmv_free(workspace);
}

/// Copy a row into a GSLVector
/// @param i :: A row index.
GSLVector GSLMatrix::copyRow(size_t i) const {
  if (i >= size1()) {
    throw std::out_of_range("GSLMatrix row index is out of range.");
  }
  auto rowView = gsl_matrix_const_row(gsl(), i);
  return GSLVector(&rowView.vector);
}

/// Copy a column into a GSLVector
/// @param i :: A column index.
GSLVector GSLMatrix::copyColumn(size_t i) const {
  if (i >= size2()) {
    throw std::out_of_range("GSLMatrix column index is out of range.");
  }
  auto columnView = gsl_matrix_const_column(gsl(), i);
  return GSLVector(&columnView.vector);
}

/// Create a new matrix and move the data to it.
GSLMatrix GSLMatrix::move() {
  return GSLMatrix(std::move(m_data), size1(), size2());
}

GSLVector GSLMatrix::multiplyByVector(const GSLVector &v) const {
  GSLVector res(size2());
  gsl_blas_dgemv(CblasNoTrans, 1.0, gsl(), v.gsl(), 1.0, res.gsl());
  return res;
}

} // namespace CurveFitting
} // namespace Mantid
