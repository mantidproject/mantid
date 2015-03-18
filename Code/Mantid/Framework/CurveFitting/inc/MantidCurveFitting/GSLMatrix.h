#ifndef MANTID_CURVEFITTING_GSLMATRIX_H_
#define MANTID_CURVEFITTING_GSLMATRIX_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/GSLVector.h"

#include "MantidKernel/Matrix.h"

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include <vector>
#include <stdexcept>
#include <iomanip>

namespace Mantid {
namespace CurveFitting {
class GSLMatrix;

// matrix transpose helper
struct Tr {
  const GSLMatrix &matrix;
  Tr(const GSLMatrix &m) : matrix(m) {}
};

// mutrix multiplication helper
struct GSLMatrixMult2 {
  const GSLMatrix &m_1;
  const GSLMatrix &m_2;
  const bool tr1;
  const bool tr2;
  GSLMatrixMult2(const GSLMatrix &m1, const GSLMatrix &m2)
      : m_1(m1), m_2(m2), tr1(false), tr2(false) {}

  GSLMatrixMult2(const Tr &m1, const GSLMatrix &m2)
      : m_1(m1.matrix), m_2(m2), tr1(true), tr2(false) {}

  GSLMatrixMult2(const GSLMatrix &m1, const Tr &m2)
      : m_1(m1), m_2(m2.matrix), tr1(false), tr2(true) {}

  GSLMatrixMult2(const Tr &m1, const Tr &m2)
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

  GSLMatrixMult3(const Tr &m1, const GSLMatrixMult2 &mm)
      : m_1(m1.matrix), m_2(mm.m_1), m_3(mm.m_2), tr1(true), tr2(mm.tr1),
        tr3(mm.tr2) {}

  GSLMatrixMult3(const GSLMatrixMult2 &mm, const GSLMatrix &m2)
      : m_1(mm.m_1), m_2(mm.m_2), m_3(m2), tr1(mm.tr1), tr2(mm.tr2),
        tr3(false) {}

  GSLMatrixMult3(const GSLMatrixMult2 &mm, const Tr &m2)
      : m_1(mm.m_1), m_2(mm.m_2), m_3(m2.matrix), tr1(mm.tr1), tr2(mm.tr2),
        tr3(true) {}
};

/**
A wrapper around gsl_matrix. The '*' operator is overloaded to help with
matrix multiplication.

@author Roman Tolchenov, Tessella plc
@date 24/02/2012

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_CURVEFITTING_DLL GSLMatrix {
  /// The pointer to the GSL matrix
  gsl_matrix *m_matrix;
public:
  /// Constructor
  GSLMatrix() : m_matrix(NULL) {}
  /// Constructor
  /// @param nx :: First dimension
  /// @param ny :: Second dimension
  GSLMatrix(const size_t nx, const size_t ny) {
    m_matrix = gsl_matrix_alloc(nx, ny);
  }

  /// Copy constructor
  /// @param M :: The other matrix.
  GSLMatrix(const GSLMatrix &M) {
    m_matrix = gsl_matrix_alloc(M.size1(), M.size2());
    gsl_matrix_memcpy(m_matrix, M.gsl());
  }

  /// Create a submatrix. A submatrix is a view into the parent matrix.
  /// Lifetime of a submatrix cannot exceed the lifetime of the parent.
  /// @param M :: The parent matrix.
  /// @param row :: The first row in the submatrix.
  /// @param col :: The first column in the submatrix.
  /// @param nRows :: The number of rows in the submatrix.
  /// @param nCols :: The number of columns in the submatrix.
  GSLMatrix(const GSLMatrix &M, size_t row, size_t col, size_t nRows, size_t nCols) {
    if ( row + nRows > M.size1() || col + nCols > M.size2() )
    {
      throw std::runtime_error("Submatrix exceeds matrix size.");
    }
    auto view = gsl_matrix_const_submatrix(M.gsl(), row, col, nRows, nCols);
    m_matrix = gsl_matrix_alloc(nRows, nCols);
    gsl_matrix_memcpy(m_matrix, &view.matrix);
  }

  /// Constructor
  /// @param M :: A matrix to copy.
  GSLMatrix(const Kernel::Matrix<double>& M) {
    m_matrix = gsl_matrix_alloc(M.numRows(), M.numCols());
    for(size_t i = 0; i < size1(); ++i)
    for(size_t j = 0; j < size2(); ++j){
      set(i,j, M[i][j]);
    }
  }

  /// Create this matrix from a product of two other matrices
  /// @param mult2 :: Matrix multiplication helper object.
  GSLMatrix(const GSLMatrixMult2 &mult2) : m_matrix(NULL) {*this = mult2;}

  /// Create this matrix from a product of three other matrices
  /// @param mult3 :: Matrix multiplication helper object.
  GSLMatrix(const GSLMatrixMult3 &mult3) : m_matrix(NULL) {*this = mult3;}

  /// Destructor.
  ~GSLMatrix() {
    if (m_matrix) {
      gsl_matrix_free(m_matrix);
    }
  }

  /// Copy assignment operator
  GSLMatrix &operator=(const GSLMatrix &M) {
    resize(M.size1(), M.size2());
    gsl_matrix_memcpy(m_matrix, M.gsl());
    return *this;
  }

  /// Get the pointer to the GSL matrix
  gsl_matrix *gsl() { return m_matrix; }

  /// Get the const pointer to the GSL matrix
  const gsl_matrix *gsl() const { return m_matrix; }

  /// Is matrix empty
  bool isEmpty() const { return m_matrix == NULL; }

  /// Resize the matrix
  /// @param nx :: New first dimension
  /// @param ny :: New second dimension
  void resize(const size_t nx, const size_t ny) {
    if (m_matrix) {
      gsl_matrix_free(m_matrix);
    }
    m_matrix = gsl_matrix_alloc(nx, ny);
  }

  /// First size of the matrix
  size_t size1() const { return m_matrix ? m_matrix->size1 : 0; }

  /// Second size of the matrix
  size_t size2() const { return m_matrix ? m_matrix->size2 : 0; }

  /// set an element
  /// @param i :: The row
  /// @param j :: The column
  /// @param value :: The new vaule
  void set(size_t i, size_t j, double value) {
    if (i < m_matrix->size1 && j < m_matrix->size2)
      gsl_matrix_set(m_matrix, i, j, value);
    else {
      throw std::out_of_range("GSLMatrix indices are out of range.");
    }
  }
  /// get an element
  /// @param i :: The row
  /// @param j :: The column
  double get(size_t i, size_t j) const {
    if (i < m_matrix->size1 && j < m_matrix->size2)
      return gsl_matrix_get(m_matrix, i, j);
    throw std::out_of_range("GSLMatrix indices are out of range.");
  }

  /// Set this matrix to identity matrix
  void identity() { gsl_matrix_set_identity(m_matrix); }

  /// Set all elements to zero
  void zero() { gsl_matrix_set_zero(m_matrix); }

  /// add a matrix to this
  /// @param M :: A matrix
  GSLMatrix &operator+=(const GSLMatrix &M) {
    gsl_matrix_add(m_matrix, M.gsl());
    return *this;
  }

  /// add a constant to this matrix
  /// @param d :: A number
  GSLMatrix &operator+=(const double &d) {
    gsl_matrix_add_constant(m_matrix, d);
    return *this;
  }

  /// subtract a matrix from this
  /// @param M :: A matrix
  GSLMatrix &operator-=(const GSLMatrix &M) {
    gsl_matrix_sub(m_matrix, M.gsl());
    return *this;
  }

  /// multiply this matrix by a number
  /// @param d :: A number
  GSLMatrix &operator*=(const double &d) {
    gsl_matrix_scale(m_matrix, d);
    return *this;
  }

  /// Assign this matrix to a product of two other matrices
  /// @param mult2 :: Matrix multiplication helper object.
  GSLMatrix &operator=(const GSLMatrixMult2 &mult2);

  /// Assign this matrix to a product of three other matrices
  /// @param mult3 :: Matrix multiplication helper object.
  GSLMatrix &operator=(const GSLMatrixMult3 &mult3);

  /// Solve system of linear equations M*x == rhs, M is this matrix
  /// This matrix is destroyed.
  /// @param rhs :: The right-hand-side vector
  /// @param x :: The solution vector
  void solve(const GSLVector &rhs, GSLVector &x);

  /// Invert this matrix
  void invert();

  /// Calculate the determinant
  double det();
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
inline GSLMatrixMult2 operator*(const Tr &m1, const GSLMatrix &m2) {
  return GSLMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix
/// @param m2 :: Second matrix transposed
inline GSLMatrixMult2 operator*(const GSLMatrix &m1, const Tr &m2) {
  return GSLMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix transposed
/// @param m2 :: Second matrix transposed
inline GSLMatrixMult2 operator*(const Tr &m1, const Tr &m2) {
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
inline GSLMatrixMult3 operator*(const Tr &m, const GSLMatrixMult2 &mm) {
  return GSLMatrixMult3(m, mm);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param mm :: Product of two matrices
/// @param m :: A transposed matrix
inline GSLMatrixMult3 operator*(const GSLMatrixMult2 &mm, const Tr &m) {
  return GSLMatrixMult3(mm, m);
}

/// The << operator. Prints a matrix in rows.
inline std::ostream &operator<<(std::ostream &ostr, const GSLMatrix &m) {
  ostr << std::scientific << std::setprecision(6);
  for (size_t i = 0; i < m.size1(); ++i) {
    for (size_t j = 0; j < m.size2(); ++j) {
      ostr << std::setw(13) << m.get(i, j) << ' ';
    }
    ostr << std::endl;
  }
  return ostr;
}

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLMATRIX_H_*/
