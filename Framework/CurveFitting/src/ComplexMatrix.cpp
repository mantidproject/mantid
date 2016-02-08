//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ComplexMatrix.h"
#include <gsl/gsl_eigen.h>

namespace Mantid {
namespace CurveFitting {

/// Constructor
ComplexMatrix::ComplexMatrix() : m_matrix(nullptr) {}
/// Constructor
/// @param nx :: First dimension
/// @param ny :: Second dimension
ComplexMatrix::ComplexMatrix(const size_t nx, const size_t ny) {
  m_matrix = gsl_matrix_complex_alloc(nx, ny);
}

/// Copy constructor
/// @param M :: The other matrix.
ComplexMatrix::ComplexMatrix(const ComplexMatrix &M) {
  m_matrix = gsl_matrix_complex_alloc(M.size1(), M.size2());
  gsl_matrix_complex_memcpy(m_matrix, M.gsl());
}

/// Create a submatrix. A submatrix is a view into the parent matrix.
/// Lifetime of a submatrix cannot exceed the lifetime of the parent.
/// @param M :: The parent matrix.
/// @param row :: The first row in the submatrix.
/// @param col :: The first column in the submatrix.
/// @param nRows :: The number of rows in the submatrix.
/// @param nCols :: The number of columns in the submatrix.
ComplexMatrix::ComplexMatrix(const ComplexMatrix &M, size_t row, size_t col, size_t nRows,
                     size_t nCols) {
  if (row + nRows > M.size1() || col + nCols > M.size2()) {
    throw std::runtime_error("Submatrix exceeds matrix size.");
  }
  auto view = gsl_matrix_complex_const_submatrix(M.gsl(), row, col, nRows, nCols);
  m_matrix = gsl_matrix_complex_alloc(nRows, nCols);
  gsl_matrix_complex_memcpy(m_matrix, &view.matrix);
}

/// Constructor
/// @param M :: A matrix to copy.
//ComplexMatrix::ComplexMatrix(const Kernel::Matrix<double> &M) {
//  m_matrix = gsl_matrix_complex_alloc(M.numRows(), M.numCols());
//  for (size_t i = 0; i < size1(); ++i)
//    for (size_t j = 0; j < size2(); ++j) {
//      set(i, j, M[i][j]);
//    }
//}

/// Create this matrix from a product of two other matrices
/// @param mult2 :: Matrix multiplication helper object.
ComplexMatrix::ComplexMatrix(const ComplexMatrixMult2 &mult2) : m_matrix(nullptr) {
  *this = mult2;
}

/// Create this matrix from a product of three other matrices
/// @param mult3 :: Matrix multiplication helper object.
ComplexMatrix::ComplexMatrix(const ComplexMatrixMult3 &mult3) : m_matrix(nullptr) {
  *this = mult3;
}

/// Destructor.
ComplexMatrix::~ComplexMatrix() {
  if (m_matrix) {
    gsl_matrix_complex_free(m_matrix);
  }
}

/// Copy assignment operator
ComplexMatrix &ComplexMatrix::operator=(const ComplexMatrix &M) {
  resize(M.size1(), M.size2());
  gsl_matrix_complex_memcpy(m_matrix, M.gsl());
  return *this;
}

/// Is matrix empty
bool ComplexMatrix::isEmpty() const { return m_matrix == nullptr; }

/// Resize the matrix
/// @param nx :: New first dimension
/// @param ny :: New second dimension
void ComplexMatrix::resize(const size_t nx, const size_t ny) {
  if (m_matrix) {
    gsl_matrix_complex_free(m_matrix);
  }
  m_matrix = gsl_matrix_complex_alloc(nx, ny);
}

/// First size of the matrix
size_t ComplexMatrix::size1() const { return m_matrix ? m_matrix->size1 : 0; }

/// Second size of the matrix
size_t ComplexMatrix::size2() const { return m_matrix ? m_matrix->size2 : 0; }

/// set an element
/// @param i :: The row
/// @param j :: The column
/// @param value :: The new vaule
void ComplexMatrix::set(size_t i, size_t j, ComplexType value) {
  if (i < m_matrix->size1 && j < m_matrix->size2)
    gsl_matrix_complex_set(m_matrix, i, j, value);
  else {
    throw std::out_of_range("ComplexMatrix indices are out of range.");
  }
}

/// get an element
/// @param i :: The row
/// @param j :: The column
ComplexType ComplexMatrix::get(size_t i, size_t j) const {
  if (i < m_matrix->size1 && j < m_matrix->size2)
    return gsl_matrix_complex_get(m_matrix, i, j);
  throw std::out_of_range("ComplexMatrix indices are out of range.");
}

/// Set this matrix to identity matrix
void ComplexMatrix::identity() { gsl_matrix_complex_set_identity(m_matrix); }

/// Set all elements to zero
void ComplexMatrix::zero() { gsl_matrix_complex_set_zero(m_matrix); }

/// Set the matrix to be diagonal.
/// @param d :: Values on the diagonal.
void ComplexMatrix::diag(const ComplexVector &d) {
  const auto n = d.size();
  resize(n, n);
  zero();
  for (size_t i = 0; i < n; ++i) {
    set(i, i, d.get(i));
  }
}

/// add a matrix to this
/// @param M :: A matrix
ComplexMatrix &ComplexMatrix::operator+=(const ComplexMatrix &M) {
  gsl_matrix_complex_add(m_matrix, M.gsl());
  return *this;
}
/// add a constant to this matrix
/// @param d :: A number
ComplexMatrix &ComplexMatrix::operator+=(const ComplexType &d) {
  gsl_matrix_complex_add_constant(m_matrix, d);
  return *this;
}
/// subtract a matrix from this
/// @param M :: A matrix
ComplexMatrix &ComplexMatrix::operator-=(const ComplexMatrix &M) {
  gsl_matrix_complex_sub(m_matrix, M.gsl());
  return *this;
}
/// multiply this matrix by a number
/// @param d :: A number
ComplexMatrix &ComplexMatrix::operator*=(const ComplexType &d) {
  gsl_matrix_complex_scale(m_matrix, d);
  return *this;
}

/// Assign this matrix to a product of two other matrices
/// @param mult2 :: Matrix multiplication helper object.
ComplexMatrix &ComplexMatrix::operator=(const ComplexMatrixMult2 &mult2) {
  // sizes of the result matrix
  size_t n1 = mult2.tr1 ? mult2.m_1.size2() : mult2.m_1.size1();
  size_t n2 = mult2.tr2 ? mult2.m_2.size1() : mult2.m_2.size2();

  this->resize(n1, n2);

  CBLAS_TRANSPOSE tr1 = mult2.tr1 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr2 = mult2.tr2 ? CblasTrans : CblasNoTrans;

  // this = m_1 * m_2
  gsl_blas_zgemm(tr1, tr2, {1.0, 0.0}, mult2.m_1.gsl(), mult2.m_2.gsl(), {0.0, 0.0}, gsl());

  return *this;
}

/// Assign this matrix to a product of three other matrices
/// @param mult3 :: Matrix multiplication helper object.
ComplexMatrix &ComplexMatrix::operator=(const ComplexMatrixMult3 &mult3) {
  // sizes of the result matrix
  size_t n1 = mult3.tr1 ? mult3.m_1.size2() : mult3.m_1.size1();
  size_t n2 = mult3.tr3 ? mult3.m_3.size1() : mult3.m_3.size2();

  this->resize(n1, n2);

  // intermediate matrix
  ComplexMatrix AB(n1, mult3.m_2.size2());

  CBLAS_TRANSPOSE tr1 = mult3.tr1 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr2 = mult3.tr2 ? CblasTrans : CblasNoTrans;
  CBLAS_TRANSPOSE tr3 = mult3.tr3 ? CblasTrans : CblasNoTrans;

  // AB = m_1 * m_2
  gsl_blas_zgemm(tr1, tr2, {1.0, 0.0}, mult3.m_1.gsl(), mult3.m_2.gsl(), {0.0, 0.0},
                 AB.gsl());

  // this = AB * m_3
  gsl_blas_zgemm(CblasNoTrans, tr3, {1.0, 0.0}, AB.gsl(), mult3.m_3.gsl(), {0.0, 0.0}, gsl());

  return *this;
}

/// Solve system of linear equations M*x == rhs, M is this matrix
/// This matrix is destroyed.
/// @param rhs :: The right-hand-side vector
/// @param x :: The solution vector
void ComplexMatrix::solve(const ComplexVector &rhs, ComplexVector &x) {
  if (size1() != size2()) {
    throw std::runtime_error(
        "System of linear equations: the matrix must be square.");
  }
  size_t n = size1();
  if (rhs.size() != n) {
    throw std::runtime_error(
        "System of linear equations: right-hand side vector has wrong size.");
  }
  x.resize(n);
  int s;
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_complex_LU_decomp(gsl(), p, &s); // matrix is modified at this moment
  gsl_linalg_complex_LU_solve(gsl(), p, rhs.gsl(), x.gsl());
  gsl_permutation_free(p);
}

/// Invert this matrix
void ComplexMatrix::invert() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  size_t n = size1();
  int s;
  ComplexMatrix LU(*this);
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_complex_LU_decomp(LU.gsl(), p, &s);
  gsl_linalg_complex_LU_invert(LU.gsl(), p, this->gsl());
  gsl_permutation_free(p);
}

/// Calculate the determinant
ComplexType ComplexMatrix::det() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  size_t n = size1();
  int s;
  ComplexMatrix LU(*this);
  gsl_permutation *p = gsl_permutation_alloc(n);
  gsl_linalg_complex_LU_decomp(LU.gsl(), p, &s);
  ComplexType res = gsl_linalg_complex_LU_det(LU.gsl(), s);
  gsl_permutation_free(p);
  return res;
}

/// Calculate the eigensystem of a Hermitian matrix
/// @param eigenValues :: Output variable that receives the eigenvalues of this
/// matrix.
/// @param eigenVectors :: Output variable that receives the eigenvectors of
/// this matrix.
void ComplexMatrix::eigenSystemHermitian(GSLVector &eigenValues, ComplexMatrix &eigenVectors) {
  size_t n = size1();
  if (n != size2()) {
    throw std::runtime_error("Matrix eigenSystem: the matrix must be square.");
  }
  eigenValues.resize(n);
  eigenVectors.resize(n, n);
  auto workspace = gsl_eigen_hermv_alloc (n);
  gsl_eigen_hermv (gsl(), eigenValues.gsl(), eigenVectors.gsl(), workspace);
  gsl_eigen_hermv_free (workspace);
}

/// Copy a row into a GSLVector
/// @param i :: A row index.
ComplexVector ComplexMatrix::copyRow(size_t i) const {
  if (i >= size1()) {
    throw std::out_of_range("ComplexMatrix row index is out of range.");
  }
  auto rowView = gsl_matrix_complex_const_row(gsl(), i);
  return ComplexVector(&rowView.vector);
}

/// Copy a column into a GSLVector
/// @param i :: A column index.
ComplexVector ComplexMatrix::copyColumn(size_t i) const {
  if (i >= size2()) {
    throw std::out_of_range("ComplexMatrix column index is out of range.");
  }
  auto columnView = gsl_matrix_complex_const_column(gsl(), i);
  return ComplexVector(&columnView.vector);
}

} // namespace CurveFitting
} // namespace Mantid
