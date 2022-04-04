// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/EigenComplexVector.h"
#include "MantidCurveFitting/EigenMatrix.h"
#include "MantidCurveFitting/EigenVector.h"

#include <Eigen/Dense>

#include <complex>
#include <iomanip>
#include <stdexcept>
#include <vector>

typedef Eigen::Map<const Eigen::MatrixXcd, 0, Mantid::CurveFitting::dynamic_stride> complex_matrix_map_type;
typedef Eigen::Map<const Eigen::VectorXcd, 0, Mantid::CurveFitting::dynamic_stride> complex_vector_map_type;

namespace Mantid {
namespace CurveFitting {
class ComplexMatrix;

/**
A complex-valued matrix for linear algebra computations.
*/
class MANTID_CURVEFITTING_DLL ComplexMatrix {
public:
  /// Constructor
  ComplexMatrix();
  /// Constructor
  ComplexMatrix(const size_t nx, const size_t ny);
  /// Copy constructor
  ComplexMatrix(const ComplexMatrix &M);
  /// Create a submatrix.
  ComplexMatrix(const ComplexMatrix &M, size_t row, size_t col, size_t nRows, size_t nCols);
  /// Move constructor
  explicit ComplexMatrix(ComplexMatrix &&m) noexcept;
  /// Move constructor with EigenMatrix
  explicit ComplexMatrix(Eigen::MatrixXcd &&m) noexcept;

  /// Copy assignment operator
  ComplexMatrix &operator=(const ComplexMatrix &M);
  /// Move assignment operator
  ComplexMatrix &operator=(ComplexMatrix &&M);
  /// Copy assignment operator
  ComplexMatrix &operator=(const Eigen::MatrixXcd M);

  /// Get the reference to the eigen matrix
  inline Eigen::MatrixXcd &eigen() { return m_matrix; }
  /// Get the const pointer to the GSL matrix
  inline const Eigen::MatrixXcd eigen() const { return m_matrix; }

  /// Is matrix empty
  bool isEmpty() const;
  /// Resize the matrix
  void resize(const size_t nx, const size_t ny);
  /// First size of the matrix
  size_t size1() const;
  /// Second size of the matrix
  size_t size2() const;
  /// Set an element
  void set(size_t i, size_t j, ComplexType value);
  /// Get an element
  ComplexType get(size_t i, size_t j) const;
  /// The "index" operator
  ComplexType operator()(size_t i, size_t j) const;
  /// Get the reference to the data element
  ComplexType &operator()(size_t i, size_t j);

  /// Set this matrix to identity matrix
  void identity();
  /// Set all elements to zero
  void zero();
  /// Set the matrix to be diagonal.
  void diag(const ComplexVector &d);
  /// Add a matrix to this
  ComplexMatrix &operator+=(const ComplexMatrix &M);
  /// Add a constant to this matrix
  ComplexMatrix &operator+=(const ComplexType &d);
  /// Subtract a matrix from this
  ComplexMatrix &operator-=(const ComplexMatrix &M);
  /// Multiply this matrix by a number
  ComplexMatrix &operator*=(const ComplexType &d);
  /// Multiply this matrix by a matrix
  ComplexMatrix operator*(const EigenMatrix &m) const;
  /// Multiply this matrix by a complex matrix
  ComplexMatrix operator*(const ComplexMatrix &m) const;

  /// Copy a row into a GSLVector
  ComplexVector copyRow(size_t i) const;
  /// Copy a column into a GSLVector
  ComplexVector copyColumn(size_t i) const;
  /// Sort columns in order defined by an index array
  void sortColumns(const std::vector<size_t> &indices);

  /// Solve system of linear equations M*x == rhs, M is this matrix
  /// This matrix is destroyed.
  /// @param rhs :: The right-hand-side vector
  /// @param x :: The solution vector
  void solve(const ComplexVector &rhs, ComplexVector &x);
  /// Invert this matrix
  void invert();
  /// Calculate the determinant
  ComplexType det();
  /// Calculate the eigensystem of a Hermitian matrix
  void eigenSystemHermitian(ComplexVector &eigenValues, ComplexMatrix &eigenVectors);

  /// Get "transposed" matrix to be used in multiplications
  ComplexMatrix tr() const;
  /// Get "conjugate transposed" matrix to be used in multiplications
  ComplexMatrix ctr() const;
  /// Pack the matrix into a single std vector of doubles (for passing in and
  /// out of algorithms)
  std::vector<double> packToStdVector() const;
  /// Unpack an std vector into this matrix. Matrix size must match the size
  /// of the vector
  void unpackFromStdVector(const std::vector<double> &v);

private:
  /// The pointer to the complex Eigen matrix
  Eigen::MatrixXcd m_matrix;
};

/// The << operator. Prints a matrix in rows.
inline std::ostream &operator<<(std::ostream &ostr, const ComplexMatrix &m) {
  std::ios::fmtflags fflags(ostr.flags());
  ostr << std::scientific << std::setprecision(6);
  for (size_t i = 0; i < m.size1(); ++i) {
    for (size_t j = 0; j < m.size2(); ++j) {
      auto value = m.get(i, j);
      ostr << std::setw(28) << std::setprecision(13) << value.real() << "+" << value.imag() << "j ";
    }
    ostr << '\n';
  }
  ostr.flags(fflags);
  return ostr;
}

} // namespace CurveFitting
} // namespace Mantid
