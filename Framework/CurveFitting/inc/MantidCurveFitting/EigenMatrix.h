// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "EigenVector.h"
#include "MantidCurveFitting/DllConfig.h"

#include "EigenMatrixView.h"
#include "MantidKernel/Matrix.h"

#include "Eigen/Core"

#include <iomanip>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace CurveFitting {

class EigenMatrix;

/**
A wrapper around Eigen::Matrix.

@author Roman Tolchenov, Tessella plc
@date 24/02/2012
*/
class MANTID_CURVEFITTING_DLL EigenMatrix {
public:
  /// Constructor
  EigenMatrix() = default;
  /// Constructor
  EigenMatrix(const size_t nx, const size_t ny);
  /// Construct from an initialisation list
  EigenMatrix(std::initializer_list<std::initializer_list<double>> ilist);
  /// Copy constructor
  EigenMatrix(const EigenMatrix &M);
  /// Create a submatrix.
  EigenMatrix(EigenMatrix &M, size_t row, size_t col, size_t nRows, size_t nCols);
  /// Constructor
  EigenMatrix(const Kernel::Matrix<double> &M);

  /// Copy assignment operator
  EigenMatrix &operator=(const EigenMatrix &M);
  /// Assignment operator - Eigen::MatrixXd
  EigenMatrix &operator=(const Eigen::MatrixXd v);

  /// Get the map to Eigen matrix
  inline map_type &mutator() { return m_view.matrix_mutator(); }
  /// Get a const copy of the Eigen matrix
  inline const map_type inspector() const { return m_view.matrix_inspector(); }
  /// Get a copy of the Eigen matrix
  inline map_type copy_view() const { return m_view.matrix_copy(); }

  /// Is matrix empty
  bool isEmpty() const;
  /// Resize the matrix
  void resize(const size_t nx, const size_t ny);
  /// First size of the matrix
  size_t size1() const;
  /// Second size of the matrix
  size_t size2() const;

  /// Set an element
  void set(size_t i, size_t j, double value);
  /// Get an element
  double get(size_t i, size_t j) const;
  /// The "index" operator
  double operator()(size_t i, size_t j) const;
  /// Get the reference to the data element
  double &operator()(size_t i, size_t j);

  /// Set this matrix to identity matrix
  void identity();
  /// Set all elements to zero
  void zero();
  /// Set the matrix to be diagonal.
  void diag(const EigenVector &d);
  /// Add a matrix to this
  EigenMatrix &operator+=(const EigenMatrix &M);
  /// Add a constant to this matrix
  EigenMatrix &operator+=(const double &d);
  /// Subtract a matrix from this
  EigenMatrix &operator-=(const EigenMatrix &M);
  /// subtract a constant from this
  EigenMatrix &operator-=(const double &d);
  /// Multiply this matrix by a number
  EigenMatrix &operator*=(const double &d);
  /// Matrix by vector multiplication
  EigenVector operator*(const EigenVector &v) const;
  /// Matrix by Matrix multiplication
  EigenMatrix operator*(const EigenMatrix &m) const;

  /// Copy a row into an EigenVector
  EigenVector copyRow(size_t i) const;
  /// Copy a column into an EigenVector
  EigenVector copyColumn(size_t i) const;

  /// Solve system of linear equations M*x == rhs, M is this matrix
  /// This matrix is destroyed.
  /// @param rhs :: The right-hand-side vector
  /// @param x :: The solution vector
  void solve(EigenVector &rhs, EigenVector &x);
  /// Invert this matrix
  void invert();
  /// Calculate the determinant
  double det() const;
  /// Calculate the eigensystem of a symmetric matrix
  void eigenSystem(Eigen::VectorXcd &eigenValues, Eigen::MatrixXcd &eigenVectors);
  /// Calculate the eigensystem of a symmetric matrix
  EigenMatrix tr() const;

protected:
  /// Create a new matrix and move the data to it.
  EigenMatrix move();

private:
  /// "Move" constructor
  EigenMatrix(std::vector<double> &&data, size_t nx, size_t ny);
  /// Default element storage
  std::vector<double> m_data;
  /// The pointer to the vector
  EigenMatrix_View m_view;
  EigenVector multiplyByVector(const EigenVector &v) const;
};

/// The << operator. Prints a matrix in rows.
inline std::ostream &operator<<(std::ostream &ostr, const EigenMatrix &m) {
  std::ios::fmtflags fflags(ostr.flags());
  ostr << std::scientific << std::setprecision(6);
  for (size_t i = 0; i < m.size1(); ++i) {
    for (size_t j = 0; j < m.size2(); ++j) {
      ostr << std::setw(13) << m.get(i, j) << ' ';
    }
    ostr << '\n';
  }
  ostr.flags(fflags);
  return ostr;
}

/// The "index" operator
inline double EigenMatrix::operator()(size_t i, size_t j) const { return const_cast<EigenMatrix &>(*this)(i, j); }

/// Get the reference to the data element
inline double &EigenMatrix::operator()(size_t i, size_t j) { return m_data[j * m_view.rows() + i]; }

} // namespace CurveFitting
} // namespace Mantid
