// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_MATRIX_H_
#define MANTID_KERNEL_MATRIX_H_

#include "MantidKernel/DllConfig.h"
#include <cfloat>
#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Our aim with Matrix is to provide the definition for
// predefined types and avoid multiple copies of the same
// definition in multiple shared libraries.
// On MSVC template declarations cannot have
// both extern and __declspec(dllexport) but gcc
// requires the __attribute__((visibility("default")))
// attribute on the class definition.
#if defined(_MSC_VER)
#define KERNEL_MATRIX_CLASS_DLL
#else
#define KERNEL_MATRIX_CLASS_DLL MANTID_KERNEL_DLL
#endif

namespace Mantid {

namespace Kernel {
//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class V3D;

/**  Numerical Matrix class.     Holds a matrix of variable type and size.
Should work for real and complex objects. Carries out eigenvalue
and inversion if the matrix is square
*/
template <typename T> class KERNEL_MATRIX_CLASS_DLL Matrix {
public:
  /// Enable users to retrieve the element type
  using value_type = T;

private:
  size_t m_numRows;    ///< Number of rows    (x coordinate)
  size_t m_numColumns; ///< Number of columns (y coordinate)

  /// 1D memory allocation to be wrapped with pointers to the rows
  template <class U> using CMemoryArray = std::unique_ptr<U[]>;
  /// Pointers to rows in the raw data array to make it appear 2D
  template <typename U> using MatrixMemoryPtrs = std::unique_ptr<U *[]>;

  /// Pointer to allocated memory
  CMemoryArray<T> m_rawDataAlloc;
  /// Representation of 2D data
  MatrixMemoryPtrs<T> m_rawData;

  void lubcmp(int *, int &); ///< starts inversion process
  void lubksb(int const *, double *);
  void rotate(double const, double const, int const, int const, int const,
              int const);

public:
  Matrix(const size_t nrow = 0, const size_t ncol = 0,
         bool const makeIdentity = false);
  /** Constructor to take two vectors and multiply them to  construct a matrix.
   * (assuming that we have columns x row vector. */
  Matrix(const std::vector<T> &, const std::vector<T> &);
  /// Build square matrix from a linear vector. Throw if the vector.size() !=
  /// m_numRows * m_numRows;
  Matrix(const std::vector<T> &);
  /// Build a non-square matrix from vector and dimensions
  Matrix(const std::vector<T> &, const size_t nrow, const size_t ncol);

  Matrix(const Matrix<T> &, const size_t nrow, const size_t ncol);

  Matrix(const Matrix<T> &);
  Matrix<T> &operator=(const Matrix<T> &);
  Matrix(Matrix<T> &&) noexcept;
  Matrix<T> &operator=(Matrix<T> &&) noexcept;

  /// const Array accessor. Use, e.g. Matrix[row][col]
  const T *operator[](const size_t row) const { return m_rawData[row]; }

  /// Array accessor. Use, e.g. Matrix[row][col]
  T *operator[](const size_t row) { return m_rawData[row]; };

  Matrix<T> &operator+=(const Matrix<T> &);     ///< Basic addition operator
  Matrix<T> operator+(const Matrix<T> &) const; ///< Basic addition operator

  Matrix<T> &operator-=(const Matrix<T> &);     ///< Basic subtraction operator
  Matrix<T> operator-(const Matrix<T> &) const; ///< Basic subtraction operator

  Matrix<T> operator*(const Matrix<T> &)const; ///< Basic matrix multiply
  std::vector<T> operator*(const std::vector<T> &)const; ///< Multiply M*Vec
  void multiplyPoint(const std::vector<T> &in,
                     std::vector<T> &out) const; ///< Multiply M*Vec
  V3D operator*(const V3D &)const;               ///< Multiply M*Vec
  Matrix<T> operator*(const T &)const;           ///< Multiply by constant

  Matrix<T> &operator*=(const Matrix<T> &); ///< Basic matrix multipy
  Matrix<T> &operator*=(const T &);         ///< Multiply by constant
  Matrix<T> &operator/=(const T &);         ///< Divide by constant

  bool operator<(const Matrix<T> &) const;
  bool operator>=(const Matrix<T> &) const;
  bool operator!=(const Matrix<T> &) const;
  bool operator==(const Matrix<T> &) const;
  bool equals(const Matrix<T> &A, const double Tolerance = FLT_EPSILON) const;
  T item(size_t row, size_t col) const { return m_rawData[row][col]; }

  void print() const;
  void write(std::ostream &, int const = 0) const;
  std::string str() const;

  // returns this matrix in 1D vector representation
  std::vector<T> getVector() const;
  // explicit conversion into the vector
  operator std::vector<T>() const {
    std::vector<T> tmp = this->getVector();
    return tmp;
  }
  //
  void setColumn(const size_t nCol, const std::vector<T> &newCol);
  void setRow(const size_t nRow, const std::vector<T> &newRow);
  void zeroMatrix(); ///< Set the matrix to zero
  void identityMatrix();
  void setRandom(size_t seed = 0, double rMin = -1,
                 double rMax = 1); ///< initialize random matrix;
  void normVert();                 ///< Vertical normalisation
  T Trace() const;                 ///< Trace of the matrix

  std::vector<T> Diagonal() const; ///< Returns a vector of the diagonal
  Matrix<T>
  preMultiplyByDiagonal(const std::vector<T> &) const; ///< pre-multiply D*this
  Matrix<T> postMultiplyByDiagonal(
      const std::vector<T> &) const; ///< post-multiply this*D

  void setMem(const size_t, const size_t);

  /// Access matrix sizes
  std::pair<size_t, size_t> size() const {
    return std::pair<size_t, size_t>(m_numRows, m_numColumns);
  }

  /// Return the number of rows in the matrix
  size_t numRows() const { return m_numRows; }

  /// Return the number of columns in the matrix
  size_t numCols() const { return m_numColumns; }

  /// Return the smallest matrix size
  size_t Ssize() const {
    return (m_numRows > m_numColumns) ? m_numColumns : m_numRows;
  }

  void swapRows(const size_t, const size_t); ///< Swap rows (first V index)
  void swapCols(const size_t, const size_t); ///< Swap cols (second V index)

  T Invert();                                      ///< LU inversion routine
  void averSymmetric();                            ///< make Matrix symmetric
  int Diagonalise(Matrix<T> &, Matrix<T> &) const; ///< (only Symmetric matrix)
  void sortEigen(Matrix<T> &);                     ///< Sort eigenvectors
  Matrix<T> Tprime() const;                        ///< Transpose the matrix
  Matrix<T> &Transpose();                          ///< Transpose the matrix

  T factor();            ///< Calculate the factor
  T determinant() const; ///< Calculate the determinant

  void GaussJordan(Matrix<T> &); ///< Create a Gauss-Jordan Inversion
  T compSum() const;

  // Check if a rotation matrix
  bool isRotation() const;
  // Check if orthogonal
  bool isOrthogonal() const;
  // Transform to a rotation matrix
  std::vector<T> toRotation();

private:
  template <typename TYPE>
  friend void dumpToStream(std::ostream &, const Kernel::Matrix<TYPE> &,
                           const char);
  template <typename TYPE>
  friend void fillFromStream(std::istream &, Kernel::Matrix<TYPE> &,
                             const char);
};

template <>
inline void Matrix<int>::GaussJordan(Kernel::Matrix<int> &)
/**
Not valid for Integer
@throw std::invalid_argument
*/
{
  throw std::invalid_argument(
      "Gauss-Jordan inversion not valid for integer matrix");
}

// Explicit declarations required by Visual C++. Symbols provided by matching
// explicit instantiations in source file
#if defined(_MSC_VER)
extern template class Matrix<double>;
extern template class Matrix<int>;
extern template class Matrix<float>;
#endif

// clean up
#undef KERNEL_MATRIX_CLASS_DLL

//-------------------------------------------------------------------------
// Typedefs
//-------------------------------------------------------------------------
using DblMatrix = Mantid::Kernel::Matrix<double>;
using IntMatrix = Mantid::Kernel::Matrix<int>;
using FloatMatrix = Mantid::Kernel::Matrix<float>;

//-------------------------------------------------------------------------
// Utility methods
//-------------------------------------------------------------------------
template <typename T>
std::ostream &operator<<(std::ostream &, const Kernel::Matrix<T> &);
template <typename T>
void dumpToStream(std::ostream &, const Kernel::Matrix<T> &, const char);

template <typename T>
std::istream &operator>>(std::istream &, Kernel::Matrix<T> &);
template <typename T>
void fillFromStream(std::istream &, Kernel::Matrix<T> &, const char);
} // namespace Kernel
} // namespace Mantid
#endif // MANTID_KERNEL_MATRIX_H_