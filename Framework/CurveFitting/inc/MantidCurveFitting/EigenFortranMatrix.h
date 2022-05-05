// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cstddef>
#include <stdexcept>

namespace Mantid {
namespace CurveFitting {

/** FortranMatrix is a wrapper template for EigenMatrix  and EigenComplexMatrix
  to simplify porting fortran programs to C++.
  This matrix allows to use arbitrary index bases as they do in
  fortran. Indexing can begin with any integer number including
  negative.
*/
template <class MatrixClass> class FortranMatrix : public MatrixClass {
  /// Base for the first index
  int m_base1;
  /// Base for the second index
  int m_base2;
  /// Typedef the types returned by the base class's operators []. They aren't
  /// necessarily the same as the stored type (double or complex).
  using ElementConstType = decltype(std::declval<const MatrixClass>().operator()(0, 0));
  using ElementRefType = decltype(std::declval<MatrixClass>().operator()(0, 0));

public:
  /// Constructor
  FortranMatrix();
  /// Constructor
  FortranMatrix(const int nx, const int ny);
  /// Constructor
  FortranMatrix(const int iFrom, const int iTo, const int jFrom, const int jTo);
  /// Resize the matrix.
  void allocate(const int iFrom, const int iTo, const int jFrom, const int jTo);
  /// Resize the matrix.
  void allocate(const int nx, const int ny);
  /// Get the size along the first dimension as an int.
  int len1() const;
  /// Get the size along the second dimension as an int.
  int len2() const;
  /// Index operator
  ElementConstType operator()(int i, int j) const;
  ElementRefType operator()(int i, int j);
  /// Assignment operator - Matrix Class
  FortranMatrix<MatrixClass> &operator=(const MatrixClass &m);

  /// Move the data to a new matrix of MatrixClass
  MatrixClass moveToBaseMatrix();
  /// copy and transpose the matrix
  FortranMatrix<MatrixClass> transpose() const;

private:
  /// Calculate the size (1D) of a matrix First
  static size_t makeSize(int firstIndex, int lastIndex);
};

//-----------------  Method implementations -------------------------//

/// Calculate the size (1D) of a matrix First
template <class MatrixClass> size_t FortranMatrix<MatrixClass>::makeSize(int firstIndex, int lastIndex) {
  if (lastIndex < firstIndex) {
    throw std::invalid_argument("Matrix defined with invalid index range.");
  }
  return static_cast<size_t>(lastIndex - firstIndex + 1);
}

/// Constructor
template <class MatrixClass>
FortranMatrix<MatrixClass>::FortranMatrix()
    : MatrixClass(this->makeSize(1, 1), this->makeSize(1, 1)), m_base1(1), m_base2(1) {}

/// Constructor
template <class MatrixClass>
FortranMatrix<MatrixClass>::FortranMatrix(const int nx, const int ny)
    : MatrixClass(this->makeSize(1, nx), this->makeSize(1, ny)), m_base1(1), m_base2(1) {}

/// Construct a FortranMatrix that has arbitrary index bases.
/// For example FortranMatrix(1,5, -2,2) creates a 5 x 5 matrix.
/// When accessing elements through operator(i,j) the first index
/// must be in the range 1 <= i <= 5 and the second in the range
/// -2 <= j <= 2.
/// The index ranges defined by this constructor apply only to
/// operator () but not to methods get() and set().
/// @param iFirst :: Lowest value for the first index
/// @param iLast :: Highest value for the first index
/// @param jFirst :: Lowest value for the second index
/// @param jLast :: Highest value for the second index
template <class MatrixClass>
FortranMatrix<MatrixClass>::FortranMatrix(const int iFirst, const int iLast, const int jFirst, const int jLast)
    : MatrixClass(this->makeSize(iFirst, iLast), this->makeSize(jFirst, jLast)), m_base1(iFirst), m_base2(jFirst) {}

/// Resize the matrix.
/// @param iFirst :: Lowest value for the first index
/// @param iLast :: Highest value for the first index
/// @param jFirst :: Lowest value for the second index
/// @param jLast :: Highest value for the second index
template <class MatrixClass>
void FortranMatrix<MatrixClass>::allocate(const int iFirst, const int iLast, const int jFirst, const int jLast) {
  m_base1 = iFirst;
  m_base2 = jFirst;
  this->resize(this->makeSize(iFirst, iLast), this->makeSize(jFirst, jLast));
}

/// Resize the matrix. The index bases are 1.
/// @param nx :: New size along the first index.
/// @param ny :: New size along the second index.
template <class MatrixClass> void FortranMatrix<MatrixClass>::allocate(const int nx, const int ny) {
  m_base1 = 1;
  m_base2 = 1;
  this->resize(this->makeSize(1, nx), this->makeSize(1, ny));
}

/// The "index" operator
template <class MatrixClass>
typename FortranMatrix<MatrixClass>::ElementConstType FortranMatrix<MatrixClass>::operator()(int i, int j) const {
  return this->MatrixClass::operator()(static_cast<size_t>(i - m_base1), static_cast<size_t>(j - m_base2));
}

/// Get the reference to the data element
template <class MatrixClass>
typename FortranMatrix<MatrixClass>::ElementRefType FortranMatrix<MatrixClass>::operator()(int i, int j) {
  return this->MatrixClass::operator()(static_cast<size_t>(i - m_base1), static_cast<size_t>(j - m_base2));
}

/// Move the data to a new matrix of MatrixClass
template <class MatrixClass> MatrixClass FortranMatrix<MatrixClass>::moveToBaseMatrix() {
  return MatrixClass(std::move(*this));
}

/// Get the size along the first dimension as an int.
template <class MatrixClass> int FortranMatrix<MatrixClass>::len1() const { return static_cast<int>(this->size1()); }

/// Get the size along the second dimension as an int.
template <class MatrixClass> int FortranMatrix<MatrixClass>::len2() const { return static_cast<int>(this->size2()); }

/// Copy matrix, transpose, then return transposed copy.
template <class MatrixClass> FortranMatrix<MatrixClass> FortranMatrix<MatrixClass>::transpose() const {
  FortranMatrix<MatrixClass> res;
  res = this->tr();
  return res;
}

/// Assignment operator - Matrix Class
template <class MatrixClass> FortranMatrix<MatrixClass> &FortranMatrix<MatrixClass>::operator=(const MatrixClass &m) {
  this->resize(m.size1(), m.size2());
  for (size_t i = 0; i < this->size1(); i++) {
    for (size_t j = 0; j < this->size2(); j++) {
      this->operator()((int)i + 1, (int)j + 1) = m.get(i, j);
    }
  }
  return *this;
}

} // namespace CurveFitting
} // namespace Mantid
