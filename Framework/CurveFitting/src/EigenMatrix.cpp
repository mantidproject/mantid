// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/EigenMatrix.h"
#include <iostream>

namespace Mantid::CurveFitting {

/// Constructor
/// @param nx :: First dimension
/// @param ny :: Second dimension
EigenMatrix::EigenMatrix(const size_t nx, const size_t ny)
    : m_data(nx * ny), m_view(EigenMatrix_View(m_data.data(), nx, ny)) {}

/// Construct from an initialisation list
/// @param ilist :: Initialisation list as a list of rows:
///      {{M00, M01, M02, ...},
///       {M10, M11, M12, ...},
///             ...
///       {Mn0, Mn1, Mn2, ...}}
EigenMatrix::EigenMatrix(std::initializer_list<std::initializer_list<double>> ilist)
    : EigenMatrix(ilist.size(), ilist.begin()->size()) {
  for (auto row = ilist.begin(); row != ilist.end(); ++row) { // loop through row by row
    if (row->size() != size2()) {                             // check number of cells in row equals number of cols
      throw std::runtime_error("All rows in initializer list must have the same size.");
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
EigenMatrix::EigenMatrix(const EigenMatrix &M)
    : m_data(M.m_data), m_view(EigenMatrix_View(m_data.data(), M.size1(), M.size2())) {}

// CHECK IF THIS WORKS
/// Create a submatrix. A submatrix is a reference to part of the parent matrix.
/// @param M :: The parent matrix.
/// @param row :: The first row in the submatrix.
/// @param col :: The first column in the submatrix.
/// @param nRows :: The number of rows in the submatrix.
/// @param nCols :: The number of columns in the submatrix.
EigenMatrix::EigenMatrix(EigenMatrix &M, size_t row, size_t col, size_t nRows, size_t nCols) {
  if (row + nRows > M.size1() || col + nCols > M.size2()) {
    throw std::runtime_error("Submatrix exceeds matrix size.");
  }
  m_data.resize(nRows * nCols);
  m_view = EigenMatrix_View(M.mutator(), nRows, nCols, row, col);
  std::memcpy(m_data.data(), M.mutator().data(), sizeof m_data.data());
}

/// Constructor
/// @param M :: A matrix to copy.
EigenMatrix::EigenMatrix(const Kernel::Matrix<double> &M)
    : m_data(M.getVector()), m_view(EigenMatrix_View(m_data.data(), M.numRows(), M.numCols())) {}

/// Create a submatrix. A submatrix is a reference to part of the parent matrix.
/// @param M :: The parent matrix.
/// @param row :: The first row in the submatrix.
/// @param col :: The first column in the submatrix.
/// @param nRows :: The number of rows in the submatrix.
/// @param nCols :: The number of columns in the submatrix.
EigenMatrix::EigenMatrix(const Kernel::Matrix<double> &M, size_t row, size_t col, size_t nRows, size_t nCols) {
  if (row + nRows > M.numRows() || col + nCols > M.numCols()) {
    throw std::runtime_error("Submatrix exceeds matrix size.");
  }
  m_data.resize(nRows * nCols);

  auto temp_view = EigenMatrix_View(M.getVector().data(), nRows, nCols, row, col);
  std::memcpy(m_data.data(), temp_view.matrix_mutator().data(), sizeof m_data.data());
}

/// "Move" constructor
EigenMatrix::EigenMatrix(std::vector<double> &&data, size_t nx, size_t ny)
    : m_data(std::move(data)), m_view(EigenMatrix_View(m_data.data(), nx, ny)) {}

/// Copy assignment operator
EigenMatrix &EigenMatrix::operator=(const EigenMatrix &M) {
  m_data = M.m_data;
  m_view = EigenMatrix_View(m_data.data(), M.size1(), M.size2());
  return *this;
}

/// Assignment operator - Eigen::MatrixXd
EigenMatrix &EigenMatrix::operator=(const Eigen::MatrixXd m) {
  m_data.resize(m.rows() * m.cols());
  for (size_t i = 0; i < m.size(); i++) {
    m_data[i] = *(m.data() + i);
  }
  m_view = EigenMatrix_View(m_data.data(), m.rows(), m.cols());
  return *this;
}

/// Is matrix empty
bool EigenMatrix::isEmpty() const { return m_data.empty(); }

/// Resize the matrix
/// @param nx :: New first dimension
/// @param ny :: New second dimension
void EigenMatrix::resize(const size_t nx, const size_t ny) {
  if (nx == 0 && ny == 0) {
    // Matrix as minimum is 1x1, retained from gsl for consistency.
    m_data.resize(2);
    m_view = EigenMatrix_View(m_data.data(), 1, 1);

  } else {
    m_data.resize(nx * ny);
    m_view = EigenMatrix_View(m_data.data(), nx, ny);
  }
}

/// First size of the matrix
size_t EigenMatrix::size1() const { return m_view.rows(); }

/// Second size of the matrix
size_t EigenMatrix::size2() const { return m_view.cols(); }

/// set an element
/// @param i :: The row
/// @param j :: The column
/// @param value :: The new vaule
void EigenMatrix::set(size_t i, size_t j, double value) {
  if (isEmpty()) {
    throw std::out_of_range("Matrix is empty.");
  }
  if (i < m_view.rows() && j < m_view.cols())
    m_view.matrix_mutator()(i, j) = value;
  else {
    throw std::out_of_range("EigenMatrix indices are out of range.");
  }
}

/// get an element
/// @param i :: The row
/// @param j :: The column
double EigenMatrix::get(size_t i, size_t j) const {
  if (isEmpty()) {
    throw std::out_of_range("Matrix is empty.");
  }
  if (i < m_view.rows() && j < m_view.cols())
    return m_view.matrix_inspector()(i, j);
  else {
    throw std::out_of_range("EigenMatrix indices are out of range.");
  }
}

/// Set this matrix to identity matrix
void EigenMatrix::identity() { m_view.matrix_mutator().setIdentity(); }

/// Set all elements to zero
void EigenMatrix::zero() { m_view.matrix_mutator().setZero(); }

/// Set the matrix to be diagonal.
/// @param d :: Values on the diagonal.
void EigenMatrix::diag(const EigenVector &d) {
  const auto n = d.size();
  resize(n, n);
  zero();
  for (size_t i = 0; i < n; ++i) {
    set(i, i, d.get(i));
  }
}

/// add a matrix to this
/// @param M :: A matrix
EigenMatrix &EigenMatrix::operator+=(const EigenMatrix &M) {
  m_view.matrix_mutator() += M.inspector();
  return *this;
}
/// add a constant to this matrix
/// @param d :: A number
EigenMatrix &EigenMatrix::operator+=(const double &d) {
  m_view.matrix_mutator().array() += d;
  return *this;
}
/// subtract a matrix from this
/// @param M :: A matrix
EigenMatrix &EigenMatrix::operator-=(const EigenMatrix &M) {
  m_view.matrix_mutator() -= M.inspector();
  return *this;
}
/// subtract a matrix from this
/// @param M :: A matrix
EigenMatrix &EigenMatrix::operator-=(const double &d) {
  m_view.matrix_mutator().array() -= d;
  return *this;
}
/// multiply this matrix by a number
/// @param d :: A number
EigenMatrix &EigenMatrix::operator*=(const double &d) {
  m_view.matrix_mutator() *= d;
  return *this;
}

/// Matrix by vector multiplication
/// @param v :: A vector to multiply by. Must have the same size as size2().
/// @returns A vector - the result of the multiplication. Size of the returned
/// vector equals size1().
/// @throws std::invalid_argument if the input vector has a wrong size.
EigenVector EigenMatrix::operator*(const EigenVector &v) const {
  if (v.size() != size2()) {
    throw std::invalid_argument("Matrix by vector multiplication: wrong size of vector.");
  }

  EigenVector res(size1());
  res.mutator() = inspector() * v.inspector();
  return res;
}

/// Matrix by Matrix multiplication
/// @param m :: A matrix to multiply by. Must have the same size1() as size2() of this matrix.
/// @returns A Matrix - the result of the multiplication. Size of the returned
/// vector equals size2() of first matrix by size1*() of second.
/// @throws std::invalid_argument if the input matrix has a wrong size.
EigenMatrix EigenMatrix::operator*(const EigenMatrix &m) const {
  if (m.size1() != size2()) {
    throw std::invalid_argument("Matrix by matrix multiplication: matricies are of incompatible sizes.");
  }

  EigenMatrix res(m.size1(), size2());
  res.mutator() = inspector() * m.inspector();
  return res;
}

/// Solve system of linear equations M*x == rhs, M is this matrix
/// @param rhs :: The right-hand-side vector
/// @param x :: The solution vector
/// @throws std::invalid_argument if the input vectors have wrong sizes.
/// @throws std::runtime_error if Eigen cannot produce a valid solution.
void EigenMatrix::solve(EigenVector &rhs, EigenVector &x) {
  if (size1() != size2()) {
    throw std::invalid_argument("System of linear equations: the matrix must be square.");
  }
  size_t n = size1();
  if (rhs.size() != n) {
    throw std::invalid_argument("System of linear equations: right-hand side vector has wrong size.");
  }
  if (det() == 0) {
    throw std::invalid_argument("Matrix A is singular.");
  }

  Eigen::MatrixXd b = rhs.inspector();
  Eigen::ColPivHouseholderQR<Eigen::MatrixXd> dec(inspector());
  auto res = dec.solve(b);
  x = res;

  if (!rhs.inspector().isApprox(inspector() * x.inspector())) {
    throw std::runtime_error("Matrix Solution Error: solution does not exist.");
  }
}

/// Invert this matrix
void EigenMatrix::invert() {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }
  mutator() = inspector().inverse();
}

/// Calculate the determinant
double EigenMatrix::det() const {
  if (size1() != size2()) {
    throw std::runtime_error("Matrix inverse: the matrix must be square.");
  }

  return inspector().determinant();
}

/// Calculate the eigensystem of a symmetric matrix
/// @param eigenValues :: Output variable that receives the eigenvalues of this
/// matrix.
/// @param eigenVectors :: Output variable that receives the eigenvectors of
/// this matrix.
void EigenMatrix::eigenSystem(Eigen::VectorXcd &eigenValues, Eigen::MatrixXcd &eigenVectors) {
  size_t n = size1();
  if (n != size2()) {
    throw std::runtime_error("Matrix eigenSystem: the matrix must be square.");
  }

  Eigen::EigenSolver<Eigen::MatrixXd> solver;
  solver.compute(inspector());
  eigenValues = solver.eigenvalues();
  eigenVectors = solver.eigenvectors();
}

/// Copy a row into a EigenVector
/// @param i :: A row index.
EigenVector EigenMatrix::copyRow(size_t i) const {
  if (i >= size1()) {
    throw std::out_of_range("EigenMatrix row index is out of range.");
  }

  EigenMatrix_View rowView = EigenMatrix_View(copy_view().data(), size1(), size2(), 1, size1(), i, 0);
  Eigen::VectorXd rowVec = Eigen::Map<Eigen::VectorXd, 0, Eigen::InnerStride<>>(
      rowView.matrix_mutator().data(), rowView.cols(), Eigen::InnerStride<>(rowView.outerStride()));
  return EigenVector(&rowVec);
}

/// Copy a column into a EigenVector
/// @param i :: A column index.
EigenVector EigenMatrix::copyColumn(size_t i) const {
  if (i >= size2()) {
    throw std::out_of_range("EigenMatrix column index is out of range.");
  }

  EigenMatrix_View colView = EigenMatrix_View(copy_view().data(), size1(), size2(), size1(), 1, 0, i);
  Eigen::VectorXd colVec = Eigen::Map<Eigen::VectorXd, 0, Eigen::InnerStride<>>(
      colView.matrix_mutator().data(), colView.rows(), Eigen::InnerStride<>(colView.innerStride()));
  return EigenVector(&colVec);
}

/// Create a new matrix and move the data to it.
EigenMatrix EigenMatrix::move() { return EigenMatrix(std::move(m_data), size1(), size2()); }

EigenVector EigenMatrix::multiplyByVector(const EigenVector &v) const {
  if (v.size() != size2()) {
    throw std::invalid_argument("Matrix by vector multiplication: wrong size of vector.");
  }

  EigenVector res(size2());
  res.mutator() = inspector() * v.inspector();

  return res;
}

/// Copy matrix, transpose, then return transposed copy.
EigenMatrix EigenMatrix::tr() const {
  EigenMatrix res = *this;
  res.mutator() = inspector().transpose();
  return res;
}

} // namespace Mantid::CurveFitting
