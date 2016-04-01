#ifndef MANTID_CURVEFITTING_COMPLEXMATRIX_H_
#define MANTID_CURVEFITTING_COMPLEXMATRIX_H_

#include "MantidCurveFitting/ComplexVector.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/GSLVector.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>

#include <iomanip>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace CurveFitting {
class ComplexMatrix;

/// Struct helping converting complex values
/// between ComplexType and internal type of
/// ComplexMatrix
struct ComplexMatrixValueConverter {
  ComplexMatrix &m_matrix;
  size_t m_index1;
  size_t m_index2;
  ComplexMatrixValueConverter(ComplexMatrix &vector, size_t i, size_t j)
      : m_matrix(vector), m_index1(i), m_index2(j) {}
  operator ComplexType() const;
  ComplexMatrixValueConverter &operator=(const ComplexType &c);
};

// Complex matrix conjugate transpose helper
struct CTr {
  const ComplexMatrix &matrix;
  CTr(const ComplexMatrix &m) : matrix(m) {}
};

// mutrix multiplication helper
struct ComplexMatrixMult2 {
  const ComplexMatrix &m_1;
  const ComplexMatrix &m_2;
  const bool tr1;  // First matrix transposed
  const bool tr2;  // Second matrix transposed
  const bool ctr1; // First matrix conjugate transposed
  const bool ctr2; // Second matrix conjugate transposed
  ComplexMatrixMult2(const ComplexMatrix &m1, const ComplexMatrix &m2)
      : m_1(m1), m_2(m2), tr1(false), tr2(false), ctr1(false), ctr2(false) {}

  ComplexMatrixMult2(const Tr<ComplexMatrix> &m1, const ComplexMatrix &m2)
      : m_1(m1.matrix), m_2(m2), tr1(true), tr2(false), ctr1(false),
        ctr2(false) {}

  ComplexMatrixMult2(const ComplexMatrix &m1, const Tr<ComplexMatrix> &m2)
      : m_1(m1), m_2(m2.matrix), tr1(false), tr2(true), ctr1(false),
        ctr2(false) {}

  ComplexMatrixMult2(const Tr<ComplexMatrix> &m1, const Tr<ComplexMatrix> &m2)
      : m_1(m1.matrix), m_2(m2.matrix), tr1(true), tr2(true), ctr1(false),
        ctr2(false) {}

  ComplexMatrixMult2(const CTr &m1, const ComplexMatrix &m2)
      : m_1(m1.matrix), m_2(m2), tr1(false), tr2(false), ctr1(true),
        ctr2(false) {}

  ComplexMatrixMult2(const ComplexMatrix &m1, const CTr &m2)
      : m_1(m1), m_2(m2.matrix), tr1(false), tr2(false), ctr1(false),
        ctr2(true) {}

  ComplexMatrixMult2(const Tr<ComplexMatrix> &m1, const CTr &m2)
      : m_1(m1.matrix), m_2(m2.matrix), tr1(true), tr2(false), ctr1(false),
        ctr2(true) {}

  ComplexMatrixMult2(const CTr &m1, const Tr<ComplexMatrix> &m2)
      : m_1(m1.matrix), m_2(m2.matrix), tr1(false), tr2(true), ctr1(true),
        ctr2(false) {}

  ComplexMatrixMult2(const CTr &m1, const CTr &m2)
      : m_1(m1.matrix), m_2(m2.matrix), tr1(false), tr2(false), ctr1(true),
        ctr2(true) {}
};

// mutrix multiplication helper
struct ComplexMatrixMult3 {
  const ComplexMatrix &m_1;
  const ComplexMatrix &m_2;
  const ComplexMatrix &m_3;
  const bool tr1;  // First matrix transposed
  const bool tr2;  // Second matrix transposed
  const bool tr3;  // Third matrix transposed
  const bool ctr1; // First matrix conjugate transposed
  const bool ctr2; // Second matrix conjugate transposed
  const bool ctr3; // Third matrix conjugate transposed
  ComplexMatrixMult3(const ComplexMatrix &m1, const ComplexMatrixMult2 &mm)
      : m_1(m1), m_2(mm.m_1), m_3(mm.m_2), tr1(false), tr2(mm.tr1), tr3(mm.tr2),
        ctr1(false), ctr2(mm.ctr1), ctr3(mm.ctr2) {}

  ComplexMatrixMult3(const Tr<ComplexMatrix> &m1, const ComplexMatrixMult2 &mm)
      : m_1(m1.matrix), m_2(mm.m_1), m_3(mm.m_2), tr1(true), tr2(mm.tr1),
        tr3(mm.tr2), ctr1(false), ctr2(mm.ctr1), ctr3(mm.ctr2) {}

  ComplexMatrixMult3(const ComplexMatrixMult2 &mm, const ComplexMatrix &m2)
      : m_1(mm.m_1), m_2(mm.m_2), m_3(m2), tr1(mm.tr1), tr2(mm.tr2), tr3(false),
        ctr1(mm.ctr1), ctr2(mm.ctr2), ctr3(false) {}

  ComplexMatrixMult3(const ComplexMatrixMult2 &mm, const Tr<ComplexMatrix> &m2)
      : m_1(mm.m_1), m_2(mm.m_2), m_3(m2.matrix), tr1(mm.tr1), tr2(mm.tr2),
        tr3(true), ctr1(mm.ctr1), ctr2(mm.ctr2), ctr3(false) {}

  ComplexMatrixMult3(const CTr &m1, const ComplexMatrixMult2 &mm)
      : m_1(m1.matrix), m_2(mm.m_1), m_3(mm.m_2), tr1(false), tr2(mm.tr1),
        tr3(mm.tr2), ctr1(true), ctr2(mm.ctr1), ctr3(mm.ctr2) {}

  ComplexMatrixMult3(const ComplexMatrixMult2 &mm, const CTr &m2)
      : m_1(mm.m_1), m_2(mm.m_2), m_3(m2.matrix), tr1(mm.tr1), tr2(mm.tr2),
        tr3(false), ctr1(mm.ctr1), ctr2(mm.ctr2), ctr3(true) {}
};

/**
A complex-valued matrix for linear algebra computations.
The '*' operator is overloaded to help with
matrix multiplication.

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
class MANTID_CURVEFITTING_DLL ComplexMatrix {
public:
  /// Constructor
  ComplexMatrix();
  /// Constructor
  ComplexMatrix(const size_t nx, const size_t ny);
  /// Copy constructor
  ComplexMatrix(const ComplexMatrix &M);
  /// Move constructor
  ComplexMatrix(ComplexMatrix &&M);
  /// Create a submatrix.
  ComplexMatrix(const ComplexMatrix &M, size_t row, size_t col, size_t nRows,
                size_t nCols);
  /// Create this matrix from a product of two other matrices
  ComplexMatrix(const ComplexMatrixMult2 &mult2);
  /// Create this matrix from a product of three other matrices
  ComplexMatrix(const ComplexMatrixMult3 &mult3);
  /// Destructor.
  ~ComplexMatrix();

  /// Copy assignment operator
  ComplexMatrix &operator=(const ComplexMatrix &M);
  /// Move assignment operator
  ComplexMatrix &operator=(ComplexMatrix &&M);
  /// Copy assignment operator
  ComplexMatrix &operator=(const gsl_matrix_complex *M);

  /// Get the pointer to the GSL matrix
  gsl_matrix_complex *gsl() { return m_matrix; }
  /// Get the const pointer to the GSL matrix
  const gsl_matrix_complex *gsl() const { return m_matrix; }

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
  /// Get a "const reference" to an element.
  const ComplexMatrixValueConverter operator()(size_t i, size_t j) const {
    return ComplexMatrixValueConverter(const_cast<ComplexMatrix &>(*this), i,
                                       j);
  }
  /// Get a "reference" to an element.
  ComplexMatrixValueConverter operator()(size_t i, size_t j) {
    return ComplexMatrixValueConverter(*this, i, j);
  }

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
  /// Assign this matrix to a product of two other matrices
  /// @param mult2 :: Matrix multiplication helper object.
  ComplexMatrix &operator=(const ComplexMatrixMult2 &mult2);
  /// Assign this matrix to a product of three other matrices
  /// @param mult3 :: Matrix multiplication helper object.
  ComplexMatrix &operator=(const ComplexMatrixMult3 &mult3);

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
  void eigenSystemHermitian(GSLVector &eigenValues,
                            ComplexMatrix &eigenVectors);

  /// Get "transposed" matrix to be used in multiplications
  Tr<ComplexMatrix> tr() { return Tr<ComplexMatrix>(*this); }
  /// Get "conjugate transposed" matrix to be used in multiplications
  CTr ctr() { return CTr(*this); }
  /// Pack the matrix into a single std vector of doubles (for passing in and
  /// out of algorithms)
  std::vector<double> packToStdVector() const;
  /// Unpack an std vector into this matrix. Matrix size must match the size
  /// of the vector
  void unpackFromStdVector(const std::vector<double> &v);

protected:
  /// Create a new matrix and move the data to it.
  ComplexMatrix move();

private:
  /// Move constructor
  ComplexMatrix(gsl_matrix_complex *&&gslMatrix);
  /// The pointer to the GSL matrix
  gsl_matrix_complex *m_matrix;
};

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix
/// @param m2 :: Second matrix
inline ComplexMatrixMult2 operator*(const ComplexMatrix &m1,
                                    const ComplexMatrix &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix transposed
/// @param m2 :: Second matrix
inline ComplexMatrixMult2 operator*(const Tr<ComplexMatrix> &m1,
                                    const ComplexMatrix &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix conjugate transposed
/// @param m2 :: Second matrix
inline ComplexMatrixMult2 operator*(const CTr &m1, const ComplexMatrix &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix
/// @param m2 :: Second matrix transposed
inline ComplexMatrixMult2 operator*(const ComplexMatrix &m1,
                                    const Tr<ComplexMatrix> &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix transposed
/// @param m2 :: Second matrix transposed
inline ComplexMatrixMult2 operator*(const Tr<ComplexMatrix> &m1,
                                    const Tr<ComplexMatrix> &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix conjugate transposed
/// @param m2 :: Second matrix transposed
inline ComplexMatrixMult2 operator*(const CTr &m1,
                                    const Tr<ComplexMatrix> &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix
/// @param m2 :: Second matrix conjugate transposed
inline ComplexMatrixMult2 operator*(const ComplexMatrix &m1, const CTr &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix transposed
/// @param m2 :: Second matrix conjugate transposed
inline ComplexMatrixMult2 operator*(const Tr<ComplexMatrix> &m1,
                                    const CTr &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication
/// @param m1 :: First matrix conjugate transposed
/// @param m2 :: Second matrix conjugate transposed
inline ComplexMatrixMult2 operator*(const CTr &m1, const CTr &m2) {
  return ComplexMatrixMult2(m1, m2);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param m :: A matrix
/// @param mm :: Product of two matrices
inline ComplexMatrixMult3 operator*(const ComplexMatrix &m,
                                    const ComplexMatrixMult2 &mm) {
  return ComplexMatrixMult3(m, mm);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param mm :: Product of two matrices
/// @param m :: A matrix
inline ComplexMatrixMult3 operator*(const ComplexMatrixMult2 &mm,
                                    const ComplexMatrix &m) {
  return ComplexMatrixMult3(mm, m);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param m :: A transposed matrix
/// @param mm :: Product of two matrices
inline ComplexMatrixMult3 operator*(const Tr<ComplexMatrix> &m,
                                    const ComplexMatrixMult2 &mm) {
  return ComplexMatrixMult3(m, mm);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param mm :: Product of two matrices
/// @param m :: A transposed matrix
inline ComplexMatrixMult3 operator*(const ComplexMatrixMult2 &mm,
                                    const Tr<ComplexMatrix> &m) {
  return ComplexMatrixMult3(mm, m);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param m :: A conjugate transposed matrix
/// @param mm :: Product of two matrices
inline ComplexMatrixMult3 operator*(const CTr &m,
                                    const ComplexMatrixMult2 &mm) {
  return ComplexMatrixMult3(m, mm);
}

/// Overloaded operator for matrix multiplication. Multiplies a matrix by a
/// product of two other matrices.
/// @param mm :: Product of two matrices
/// @param m :: A conjugate transposed matrix
inline ComplexMatrixMult3 operator*(const ComplexMatrixMult2 &mm,
                                    const CTr &m) {
  return ComplexMatrixMult3(mm, m);
}

/// The << operator. Prints a matrix in rows.
inline std::ostream &operator<<(std::ostream &ostr, const ComplexMatrix &m) {
  std::ios::fmtflags fflags(ostr.flags());
  ostr << std::scientific << std::setprecision(6);
  for (size_t i = 0; i < m.size1(); ++i) {
    for (size_t j = 0; j < m.size2(); ++j) {
      auto value = m.get(i, j);
      ostr << std::setw(28) << std::setprecision(13) << value.real() << "+"
           << value.imag() << "j ";
    }
    ostr << std::endl;
  }
  ostr.flags(fflags);
  return ostr;
}

/// Convert an internal complex value (GSL type) to ComplexType.
inline ComplexMatrixValueConverter::operator ComplexType() const {
  return m_matrix.get(m_index1, m_index2);
}

/// Convert a value of ComplexType to the internal complex value (GSL type).
inline ComplexMatrixValueConverter &ComplexMatrixValueConverter::
operator=(const ComplexType &c) {
  m_matrix.set(m_index1, m_index2, c);
  return *this;
}

/// Equality operator
inline bool operator==(const ComplexType &c,
                       const ComplexMatrixValueConverter &conv) {
  return c == static_cast<ComplexType>(conv);
}

/// Equality operator
inline bool operator==(const ComplexMatrixValueConverter &conv,
                       const ComplexType &c) {
  return c == static_cast<ComplexType>(conv);
}

/// Inequality operator
inline bool operator!=(const ComplexType &c,
                       const ComplexMatrixValueConverter &conv) {
  return c != static_cast<ComplexType>(conv);
}

/// Inequality operator
inline bool operator!=(const ComplexMatrixValueConverter &conv,
                       const ComplexType &c) {
  return c != static_cast<ComplexType>(conv);
}

/// Plus operator
inline ComplexType operator+(const ComplexMatrixValueConverter &conv,
                             const ComplexType &c) {
  return static_cast<ComplexType>(conv) + c;
}

/// Minus operator
inline ComplexType operator-(const ComplexMatrixValueConverter &conv,
                             const ComplexType &c) {
  return static_cast<ComplexType>(conv) - c;
}

/// Multiplication operator
inline ComplexType operator*(const ComplexMatrixValueConverter &conv,
                             const ComplexType &c) {
  return static_cast<ComplexType>(conv) * c;
}

/// Division operator
inline ComplexType operator/(const ComplexMatrixValueConverter &conv,
                             const ComplexType &c) {
  return static_cast<ComplexType>(conv) / c;
}

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COMPLEXMATRIX_H_*/
