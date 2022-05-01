// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/EigenComplexMatrix.h"

namespace Mantid::CurveFitting {

/// Constructor
ComplexMatrix::ComplexMatrix() {}
/// Constructor
/// @param nx :: First dimension
/// @param ny :: Second dimension
ComplexMatrix::ComplexMatrix(const size_t nx, const size_t ny) : m_matrix(Eigen::MatrixXcd(nx, ny)) { zero(); }

/// Copy constructor
/// @param M :: The other matrix.
ComplexMatrix::ComplexMatrix(const ComplexMatrix &M) : m_matrix(M.eigen()) {}

/// Create a submatrix. A submatrix is a view into the parent matrix.
/// Lifetime of a submatrix cannot exceed the lifetime of the parent.
/// @param M :: The parent matrix.
/// @param row :: The first row in the submatrix.
/// @param col :: The first column in the submatrix.
/// @param nRows :: The number of rows in the submatrix.
/// @param nCols :: The number of columns in the submatrix.
ComplexMatrix::ComplexMatrix(const ComplexMatrix &M, size_t row, size_t col, size_t nRows, size_t nCols) {
  if (row + nRows > M.size1() || col + nCols > M.size2()) {
    throw std::runtime_error("Submatrix exceeds matrix size.");
  }

  m_matrix = complex_matrix_map_type(M.eigen().data() + (col * M.size1()) + row, nRows, nCols,
                                     dynamic_stride(M.eigen().outerStride(), M.eigen().innerStride()));
}

/// Move constructor
ComplexMatrix::ComplexMatrix(ComplexMatrix &&m) noexcept : m_matrix(std::move(m.m_matrix)) {}

/// Move constructor with Eigen::Matrix
ComplexMatrix::ComplexMatrix(Eigen::MatrixXcd &&m) noexcept : m_matrix(std::move(m)) {}

/// Copy assignment operator
ComplexMatrix &ComplexMatrix::operator=(const ComplexMatrix &M) {
  m_matrix = M.eigen();
  return *this;
}

/// Move assignment operator - check this still works now m_matrix is no longer a pointer.
ComplexMatrix &ComplexMatrix::operator=(ComplexMatrix &&M) {
  m_matrix = std::move(M.m_matrix);
  return *this;
}

/// Copy assignment operator - check we don't need a non-const assignment operator.
ComplexMatrix &ComplexMatrix::operator=(const Eigen::MatrixXcd eigenMatrix) {
  m_matrix = eigenMatrix;
  return *this;
}

/// Is matrix empty
bool ComplexMatrix::isEmpty() const { return m_matrix.size() == 0 ? 1 : 0; }

/// Resize the matrix
/// @param nx :: New first dimension
/// @param ny :: New second dimension
void ComplexMatrix::resize(const size_t nx, const size_t ny) {
  if (nx == size1() && ny == size2()) {
    return;
  }
  m_matrix.resize(nx, ny);
  zero();
}

/// First size of the matrix
size_t ComplexMatrix::size1() const { return m_matrix.rows(); }

/// Second size of the matrix
size_t ComplexMatrix::size2() const { return m_matrix.cols(); }

/// set an element
/// @param i :: The row
/// @param j :: The column
/// @param value :: The new vaule
void ComplexMatrix::set(size_t i, size_t j, ComplexType value) {
  if (i < size1() && j < size2()) {
    m_matrix(i, j) = value;
  } else {
    throw std::out_of_range("ComplexMatrix indices are out of range.");
  }
}

/// get an element
/// @param i :: The row
/// @param j :: The column
ComplexType ComplexMatrix::get(size_t i, size_t j) const {
  if (i < size1() && j < size2()) {
    return m_matrix(i, j);
  }
  throw std::out_of_range("ComplexMatrix indices are out of range.");
}

/// The "index" operator
ComplexType ComplexMatrix::operator()(size_t i, size_t j) const { return const_cast<ComplexMatrix &>(*this)(i, j); }

/// Get the reference to the data element
ComplexType &ComplexMatrix::operator()(size_t i, size_t j) { return m_matrix(i, j); }

/// Set this matrix to identity matrix
void ComplexMatrix::identity() { m_matrix.setIdentity(); }

/// Set all elements to zero
void ComplexMatrix::zero() { m_matrix.setZero(); }

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
  m_matrix += M.eigen();
  return *this;
}
/// add a constant to this matrix
/// @param d :: A number
ComplexMatrix &ComplexMatrix::operator+=(const ComplexType &d) {
  m_matrix.array() += d;
  return *this;
}
/// subtract a matrix from this
/// @param M :: A matrix
ComplexMatrix &ComplexMatrix::operator-=(const ComplexMatrix &M) {
  m_matrix -= M.eigen();
  return *this;
}
/// multiply this matrix by a number
/// @param d :: A number
ComplexMatrix &ComplexMatrix::operator*=(const ComplexType &d) {
  m_matrix *= d;
  return *this;
}

/// Multiply this matrix by a matrix
ComplexMatrix ComplexMatrix::operator*(const EigenMatrix &m) const {
  if (m.size1() != size2()) {
    throw std::invalid_argument("Matrix by matrix multiplication: matricies are of incompatible sizes.");
  }

  ComplexMatrix res(m.size1(), size2());
  res.eigen() = eigen() * m.inspector();
  return res;
}

/// Multiply this matrix by a complex matrix
ComplexMatrix ComplexMatrix::operator*(const ComplexMatrix &m) const {
  if (m.size1() != size2()) {
    throw std::invalid_argument("Matrix by matrix multiplication: matricies are of incompatible sizes.");
  }

  ComplexMatrix res(m.size1(), size2());
  res.eigen() = eigen() * m.eigen();
  return res;
}

/// Solve system of linear equations M*x == rhs, M is this matrix
/// This matrix is destroyed.
/// @param rhs :: The right-hand-side vector
/// @param x :: The solution vector
void ComplexMatrix::solve(const ComplexVector &rhs, ComplexVector &x) {
  if (size1() != size2()) {
    throw std::invalid_argument("System of linear equations: the matrix must be square.");
  }
  size_t n = size1();
  if (rhs.size() != n) {
    throw std::invalid_argument("System of linear equations: right-hand side vector has wrong size.");
  }
  if (det() == ComplexType(0, 0)) {
    throw std::invalid_argument("Matrix A is singular.");
  }

  Eigen::MatrixXcd b = rhs.eigen();
  Eigen::ColPivHouseholderQR<Eigen::MatrixXcd> dec(eigen());
  Eigen::VectorXcd res = dec.solve(b);
  x = res;

  if (!rhs.eigen().isApprox(eigen() * x.eigen())) {
    throw std::runtime_error("Matrix Solution Error: solution does not exist.");
  }
}

/// Invert this matrix
void ComplexMatrix::invert() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  m_matrix = m_matrix.inverse();
}

/// Calculate the determinant
ComplexType ComplexMatrix::det() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }

  return m_matrix.determinant();
}

/// Calculate the eigensystem of a Hermitian matrix
/// @param eigenValues :: Output variable that receives the eigenvalues of this
/// matrix.
/// @param eigenVectors :: Output variable that receives the eigenvectors of
/// this matrix.
void ComplexMatrix::eigenSystemHermitian(EigenVector &eigenValues, ComplexMatrix &eigenVectors) {
  size_t n = size1();
  if (n != size2()) {
    throw std::runtime_error("Matrix eigenSystem: the matrix must be square.");
  }
  eigenValues.resize(n);

  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver;
  solver.compute(m_matrix);

  eigenValues = solver.eigenvalues();
  eigenVectors = solver.eigenvectors();
}

/// Copy a row into a EigenVector
/// @param i :: A row index.
ComplexVector ComplexMatrix::copyRow(size_t i) const {
  if (i >= size1()) {
    throw std::out_of_range("EigenMatrix row index is out of range.");
  }

  Eigen::VectorXcd row = complex_vector_map_type(eigen().data() + i, size2(), dynamic_stride(0, eigen().outerStride()));
  return ComplexVector(row);
}

/// Copy a column into a Complex EigenVector
/// @param i :: A column index.
ComplexVector ComplexMatrix::copyColumn(size_t i) const {
  if (i >= size2()) {
    throw std::out_of_range("ComplexMatrix column index is out of range.");
  }

  Eigen::VectorXcd col =
      complex_vector_map_type(eigen().data() + (i * size1()), size1(), dynamic_stride(0, eigen().innerStride()));
  return ComplexVector(col);
}

/// Sort columns in order defined by an index array
/// @param indices :: Indices defining the order of columns in sorted matrix.
void ComplexMatrix::sortColumns(const std::vector<size_t> &indices) {
  Eigen::MatrixXcd matrix = Eigen::MatrixXcd(size1(), size2());
  for (size_t col = 0; col < size2(); ++col) {
    auto col1 = indices[col];
    for (size_t row = 0; row < size1(); ++row) {
      matrix(row, col) = m_matrix(row, col1);
    }
  }
  m_matrix = matrix;
}

/// Pack the matrix into a single std vector of doubles (for passing in and out
/// of algorithms)
std::vector<double> ComplexMatrix::packToStdVector() const {
  const size_t n1 = size1();
  const size_t n2 = size2();

  std::vector<double> packed(2 * n1 * n2);
  for (size_t i = 0; i < n1; ++i) {
    for (size_t j = 0; j < n2; ++j) {
      auto k = 2 * (i * n2 + j);
      ComplexType value = get(i, j);
      packed[k] = value.real();
      packed[k + 1] = value.imag();
    }
  }
  return packed;
}

/// Unpack an std vector into this matrix. Matrix size must match the size
/// of the vector
/// @param packed :: A vector with complex data packed with
/// ComplexMatrix::packToStdVector().
void ComplexMatrix::unpackFromStdVector(const std::vector<double> &packed) {
  const size_t n1 = size1();
  const size_t n2 = size2();
  if (2 * n1 * n2 != packed.size()) {
    throw std::runtime_error("Cannot unpack vector into ComplexMatrix: size mismatch.");
  }
  for (size_t i = 0; i < n1; ++i) {
    for (size_t j = 0; j < n2; ++j) {
      auto k = 2 * (i * n2 + j);
      ComplexType value(packed[k], packed[k + 1]);
      set(i, j, value);
    }
  }
}

/// Copy matrix, transpose, then return transposed copy.
ComplexMatrix ComplexMatrix::tr() const {
  ComplexMatrix res = *this;
  res.eigen() = m_matrix.transpose();
  return res;
}

/// Copy matrix, conjugate, then return transposed copy.
ComplexMatrix ComplexMatrix::ctr() const {
  ComplexMatrix res = *this;
  res.eigen() = m_matrix.adjoint();
  return res;
}

} // namespace Mantid::CurveFitting
