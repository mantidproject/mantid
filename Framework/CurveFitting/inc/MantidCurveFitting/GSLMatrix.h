// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_GSLMATRIX_H_
#define MANTID_CURVEFITTING_GSLMATRIX_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/GSLVector.h"

#include "MantidKernel/Matrix.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>

#include <iomanip>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace CurveFitting {
class GSLMatrix;

// matrix transpose helper
template <class M> struct Tr {
  const M &matrix;
  Tr(const M &m) : matrix(m) {}
};

// mutrix multiplication helper
struct GSLMatrixMult2 {
  const GSLMatrix &m_1;
  const GSLMatrix &m_2;
  const bool tr1;
  const bool tr2;
  GSLMatrixMult2(const GSLMatrix &m1, const GSLMatrix &m2)
      : m_1(m1), m_2(m2), tr1(false), tr2(false) {}

  GSLMatrixMult2(const Tr<GSLMatrix> &m1, const GSLMatrix &m2)
      : m_1(m1.matrix), m_2(m2), tr1(true), tr2(false) {}

  GSLMatrixMult2(const GSLMatrix &m1, const Tr<GSLMatrix> &m2)
      : m_1(m1), m_2(m2.matrix), tr1(false), tr2(true) {}

  GSLMatrixMult2(const Tr<GSLMatrix> &m1, const Tr<GSLMatrix> &m2)
      : m_1(m1.matrix), m_2(m2.matrix), tr1(true), tr2(true) {}
};

// mutrix multiplication helper
struct GSLMatrixMult3 {
  const GSLMatrix &m_1;
  const GSLMatrix &m_2;
  const GSLMatrix &m_3;
  const bool tr1;
  const bool tr2;
  const bool tr3;
  GSLMatrixMult3(const GSLMatrix &m1, const GSLMatrixMult2 &mm)
      : m_1(m1), m_2(mm.m_1), m_3(mm.m_2), tr1(false), tr2(mm.tr1),
        tr3(mm.tr2) {}

  GSLMatrixMult3(const Tr<GSLMatrix> &m1, const GSLMatrixMult2 &mm)
      : m_1(m1.matrix), m_2(mm.m_1), m_3(mm.m_2), tr1(true), tr2(mm.tr1),
        tr3(mm.tr2) {}

  GSLMatrixMult3(const GSLMatrixMult2 &mm, const GSLMatrix &m2)
      : m_1(mm.m_1), m_2(mm.m_2), m_3(m2), tr1(mm.tr1), tr2(mm.tr2),
        tr3(false) {}

  GSLMatrixMult3(const GSLMatrixMult2 &mm, const Tr<GSLMatrix> &m2)
      : m_1(mm.m_1), m_2(mm.m_2), m_3(m2.matrix), tr1(mm.tr1), tr2(mm.tr2),
        tr3(true) {}
};

/**
A wrapper around gsl_matrix. The '*' operator is overloaded to help with
matrix multiplication.

@author Roman Tolchenov, Tessella plc
@date 24/02/2012
*/
class MANTID_CURVEFITTING_DLL GSLMatrix {
public:
  /// Constructor
  GSLMatrix() = default;
  /// Constructor
  GSLMatrix(const size_t nx, const size_t ny);
  /// Construct from an initialisation list
  GSLMatrix(std::initializer_list<std::initializer_list<double>> ilist);
  /// Copy constructor
  GSLMatrix(const GSLMatrix &M);
  /// Create a submatrix.
  GSLMatrix(const GSLMatrix &M, size_t row, size_t col, size_t nRows,
            size_t nCols);
  /// Constructor
  GSLMatrix(const Kernel::Matrix<double> &M);
  /// Create this matrix from a product of two other matrices
  GSLMatrix(const GSLMatrixMult2 &mult2);
  /// Create this matrix from a product of three other matrices
  GSLMatrix(const GSLMatrixMult3 &mult3);

  /// Copy assignment operator
  GSLMatrix &operator=(const GSLMatrix &M);

  /// Get the pointer to the GSL matrix
  gsl_matrix *gsl();
  /// Get the const pointer to the GSL matrix
  const gsl_matrix *gsl() const;

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
  void diag(const GSLVector &d);
  /// Add a matrix to this
  GSLMatrix &operator+=(const GSLMatrix &M);
  /// Add a constant to this matrix
  GSLMatrix &operator+=(const double &d);
  /// Subtract a matrix from this
  GSLMatrix &operator-=(const GSLMatrix &M);
  /// Multiply this matrix by a number
  GSLMatrix &operator*=(const double &d);
  /// Matrix by vector multiplication
  GSLVector operator*(const GSLVector &v) const;
  /// Assign this matrix to a product of two other matrices
  /// @param mult2 :: Matrix multiplication helper object.
  GSLMatrix &operator=(const GSLMatrixMult2 &mult2);
  /// Assign this matrix to a product of three other matrices
  /// @param mult3 :: Matrix multiplication helper object.
  GSLMatrix &operator=(const GSLMatrixMult3 &mult3);

  /// Copy a row into a GSLVector
  GSLVector copyRow(size_t i) const;
  /// Copy a column into a GSLVector
  GSLVector copyColumn(size_t i) const;

  /// Solve system of linear equations M*x == rhs, M is this matrix
  /// This matrix is destroyed.
  /// @param rhs :: The right-hand-side vector
  /// @param x :: The solution vector
  void solve(const GSLVector &rhs, GSLVector &x);
  /// Invert this matrix
  void invert();
  /// Calculate the determinant
  double det() const;
  /// Calculate the eigensystem of a symmetric matrix
  void eigenSystem(GSLVector &eigenValues, GSLMatrix &eigenVectors);
  Tr<GSLMatrix> tr() { return Tr<GSLMatrix>(*this); }

protected:
  /// Create a new matrix and move the data to it.
  GSLMatrix move();

private:
  /// "Move" constructor
  GSLMatrix(std::vector<double> &&data, size_t nx, size_t ny);
  /// Default element storage
  std::vector<double> m_data;
  /// The pointer to the GSL vector
  gsl_matrix_view m_view;
  GSLVector multiplyByVector(const GSLVector &v) const;
};

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix
/// @param m2 :: Second matrix
inline GSLMatrixMult2 operator*(const GSLMatrix &m1, const GSLMatrix &m2) {
  return GSLMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix transposed
/// @param m2 :: Second matrix
inline GSLMatrixMult2 operator*(const Tr<GSLMatrix> &m1, const GSLMatrix &m2) {
  return GSLMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix
/// @param m2 :: Second matrix transposed
inline GSLMatrixMult2 operator*(const GSLMatrix &m1, const Tr<GSLMatrix> &m2) {
  return GSLMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix transposed
/// @param m2 :: Second matrix transposed
inline GSLMatrixMult2 operator*(const Tr<GSLMatrix> &m1,
                                const Tr<GSLMatrix> &m2) {
  return GSLMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param m :: A matrix
/// @param mm :: Product of two matrices
inline GSLMatrixMult3 operator*(const GSLMatrix &m, const GSLMatrixMult2 &mm) {
  return GSLMatrixMult3(m, mm);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param mm :: Product of two matrices
/// @param m :: A matrix
inline GSLMatrixMult3 operator*(const GSLMatrixMult2 &mm, const GSLMatrix &m) {
  return GSLMatrixMult3(mm, m);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param m :: A transposed matrix
/// @param mm :: Product of two matrices
inline GSLMatrixMult3 operator*(const Tr<GSLMatrix> &m,
                                const GSLMatrixMult2 &mm) {
  return GSLMatrixMult3(m, mm);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param mm :: Product of two matrices
/// @param m :: A transposed matrix
inline GSLMatrixMult3 operator*(const GSLMatrixMult2 &mm,
                                const Tr<GSLMatrix> &m) {
  return GSLMatrixMult3(mm, m);
}

/// The << operator. Prints a matrix in rows.
inline std::ostream &operator<<(std::ostream &ostr, const GSLMatrix &m) {
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
inline double GSLMatrix::operator()(size_t i, size_t j) const {
  return const_cast<GSLMatrix &>(*this)(i, j);
}

/// Get the reference to the data element
inline double &GSLMatrix::operator()(size_t i, size_t j) {
  // This is how it works according to the GSL docs
  // https://www.gnu.org/software/gsl/manual/html_node/Matrix-views.html
  return m_data[i * m_view.matrix.size2 + j];
}

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLMATRIX_H_*/
