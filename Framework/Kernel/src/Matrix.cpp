// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Matrix.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <memory>
#include <sstream>

namespace Mantid::Kernel {
//
#define fabs(x) std::fabs((x) * 1.0)

namespace {
//-------------------------------------------------------------------------
// Utility methods and function objects in anonymous namespace
//-------------------------------------------------------------------------
/**
\class PIndex
\author S. Ansell
\date Aug 2005
\version 1.0
\brief Class  to fill an index with a progressive count
*/
template <typename T> struct PIndex {
private:
  int count; ///< counter
public:
  /// Constructor
  PIndex() : count(0) {}
  /// functional
  std::pair<T, int> operator()(const T &A) { return std::pair<T, int>(A, count++); }
};

/**
\class PSep
\author S. Ansell
\date Aug 2005
\version 1.0
\brief Class to access the second object in index pair.
*/
template <typename T> struct PSep {
  /// Functional to the second object
  int operator()(const std::pair<T, int> &A) { return A.second; }
};

/**
 * Function to take a vector and sort the vector
 * so as to produce an index. Leaves the vector unchanged.
 * @param pVec :: Input vector
 * @param Index :: Output vector
 */
template <typename T> void indexSort(const std::vector<T> &pVec, std::vector<int> &Index) {
  Index.resize(pVec.size());
  std::vector<typename std::pair<T, int>> PartList;
  PartList.resize(pVec.size());

  transform(pVec.begin(), pVec.end(), PartList.begin(), PIndex<T>());
  sort(PartList.begin(), PartList.end());
  transform(PartList.begin(), PartList.end(), Index.begin(), PSep<T>());
}
template void indexSort(const std::vector<double> &, std::vector<int> &);
template void indexSort(const std::vector<float> &, std::vector<int> &);
template void indexSort(const std::vector<int> &, std::vector<int> &);
} // namespace

template <typename T> std::vector<T> Matrix<T>::getVector() const {
  std::vector<T> rez(m_numRows * m_numColumns);
  size_t ic(0);
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      rez[ic] = m_rawData[i][j];
      ic++;
    }
  }
  return rez;
}
//
template <typename T>
Matrix<T>::Matrix(const size_t nrow, const size_t ncol, const bool makeIdentity)
    : m_numRows(0), m_numColumns(0)
/**
Constructor with pre-set sizes. Matrix is zeroed
@param nrow :: number of rows
@param ncol :: number of columns
@param makeIdentity :: flag for the constructor to return an identity matrix
*/
{
  // Note:: m_numRows, m_numColumns zeroed so setMem always works
  setMem(nrow, ncol);
  zeroMatrix();
  if (makeIdentity)
    identityMatrix();
}

template <typename T>
Matrix<T>::Matrix(const std::vector<T> &A, const std::vector<T> &B)
    : m_numRows(0), m_numColumns(0)
/**
Constructor to take two vectors and multiply them to
construct a matrix. (assuming that we have columns x row
vector.
@param A :: Column vector to multiply
@param B :: Row vector to multiply
*/
{
  // Note:: m_numRows,m_numColumns zeroed so setMem always works
  setMem(A.size(), B.size());
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      m_rawData[i][j] = A[i] * B[j];
    }
  }
}
//
template <typename T> Matrix<T>::Matrix(const std::vector<T> &data) : m_numRows(0), m_numColumns(0) {
  size_t numElements = data.size();
  auto numOfRows = static_cast<size_t>(sqrt(double(numElements)));
  size_t numRowsSquare = numOfRows * numOfRows;
  if (numElements != numRowsSquare) {
    throw(std::invalid_argument("number of elements in input vector have to be square of some value"));
  }

  setMem(numOfRows, numOfRows);

  size_t ic(0);
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      m_rawData[i][j] = data[ic];
      ic++;
    }
  }
}

template <typename T>
Matrix<T>::Matrix(const std::vector<T> &data, const size_t nrow, const size_t ncol) : m_numRows(0), m_numColumns(0) {
  size_t numel = data.size();
  size_t test = nrow * ncol;
  if (test != numel) {
    throw(std::invalid_argument("number of elements in input vector have is "
                                "incompatible with the number of rows and "
                                "columns"));
  }

  setMem(nrow, ncol);

  size_t ic(0);
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      m_rawData[i][j] = data[ic];
      ic++;
    }
  }
}

template <typename T>
Matrix<T>::Matrix(const Matrix<T> &A, const size_t nrow, const size_t ncol)
    : m_numRows(A.m_numRows - 1), m_numColumns(A.m_numColumns - 1)
/**
Constructor with for a missing row/column.
@param A :: The input matrix
@param nrow :: number of row to miss
@param ncol :: number of column to miss
*/
{
  if (nrow > m_numRows)
    throw Kernel::Exception::IndexError(nrow, A.m_numRows, "Matrix::Constructor without row");
  if (ncol > m_numColumns)
    throw Kernel::Exception::IndexError(ncol, A.m_numColumns, "Matrix::Constructor without col");
  setMem(m_numRows, m_numColumns);
  size_t iR(0);
  for (size_t i = 0; i <= m_numRows; i++) {
    if (i != nrow) {
      size_t jR(0);
      for (size_t j = 0; j <= m_numColumns; j++) {
        if (j != ncol) {
          m_rawData[iR][jR] = A.m_rawData[i][j];
          jR++;
        }
      }
      iR++;
    }
  }
}

template <typename T>
Matrix<T>::Matrix(const Matrix<T> &A)
    : m_numRows(0), m_numColumns(0)
/**
Simple copy constructor
@param A :: Object to copy
*/
{
  // Note:: m_numRows,m_numColumns zeroed so setMem always works
  setMem(A.m_numRows, A.m_numColumns);
  if ((m_numRows * m_numColumns) > 0) {
    for (size_t i = 0; i < m_numRows; i++) {
      for (size_t j = 0; j < m_numColumns; j++) {
        m_rawData[i][j] = A.m_rawData[i][j];
      }
    }
  }
}

template <typename T>
Matrix<T> &Matrix<T>::operator=(const Matrix<T> &A)
/**
Simple assignment operator
@param A :: Object to copy
@return the copied object
*/
{
  if (&A != this) {
    setMem(A.m_numRows, A.m_numColumns);
    if ((m_numRows * m_numColumns) > 0) {
      for (size_t i = 0; i < m_numRows; i++) {
        for (size_t j = 0; j < m_numColumns; j++) {
          m_rawData[i][j] = A.m_rawData[i][j];
        }
      }
    }
  }
  return *this;
}

template <typename T>
Matrix<T>::Matrix(Matrix<T> &&other) noexcept
    : m_numRows(other.m_numRows), m_numColumns(other.m_numColumns), m_rawDataAlloc(std::move(other.m_rawDataAlloc)),
      m_rawData(std::move(other.m_rawData)) {
  other.m_numRows = 0;
  other.m_numColumns = 0;
}

template <typename T> Matrix<T> &Matrix<T>::operator=(Matrix<T> &&other) noexcept {
  m_numRows = other.m_numRows;
  m_numColumns = other.m_numColumns;
  m_rawData = std::move(other.m_rawData);
  m_rawDataAlloc = std::move(other.m_rawDataAlloc);

  other.m_numRows = 0;
  other.m_numColumns = 0;

  return *this;
}

template <typename T>
Matrix<T> &Matrix<T>::operator+=(const Matrix<T> &A)
/**
Matrix addition THIS + A
If the size is different then 0 is added where appropiate
Matrix A is not expanded.
@param A :: Matrix to add
@return Matrix(this + A)
*/
{
  const size_t Xpt((m_numRows > A.m_numRows) ? A.m_numRows : m_numRows);
  const size_t Ypt((m_numColumns > A.m_numColumns) ? A.m_numColumns : m_numColumns);
  for (size_t i = 0; i < Xpt; i++) {
    for (size_t j = 0; j < Ypt; j++) {
      m_rawData[i][j] += A.m_rawData[i][j];
    }
  }

  return *this;
}

template <typename T>
Matrix<T> &Matrix<T>::operator-=(const Matrix<T> &A)
/**
Matrix subtractoin THIS - A
If the size is different then 0 is added where appropiate
Matrix A is not expanded.
@param A :: Matrix to add
@return Ma
*/
{
  const size_t Xpt((m_numRows > A.m_numRows) ? A.m_numRows : m_numRows);
  const size_t Ypt((m_numColumns > A.m_numColumns) ? A.m_numColumns : m_numColumns);
  for (size_t i = 0; i < Xpt; i++) {
    for (size_t j = 0; j < Ypt; j++) {
      m_rawData[i][j] -= A.m_rawData[i][j];
    }
  }

  return *this;
}

template <typename T>
Matrix<T> Matrix<T>::operator+(const Matrix<T> &A) const
/**
Matrix addition THIS + A
If the size is different then 0 is added where appropiate
Matrix A is not expanded.
@param A :: Matrix to add
@return Matrix(this + A)
*/
{
  Matrix<T> X(*this);
  return X += A;
}

template <typename T>
Matrix<T> Matrix<T>::operator-(const Matrix<T> &A) const
/**
Matrix subtraction THIS - A
If the size is different then 0 is subtracted where
appropiate. This matrix determines the size
@param A :: Matrix to add
@return Matrix(this + A)
*/
{
  Matrix<T> X(*this);
  return X -= A;
}

template <typename T>
Matrix<T> Matrix<T>::operator*(const Matrix<T> &A) const
/**
Matrix multiplication THIS * A
@param A :: Matrix to multiply by  (this->row must == A->columns)
@throw MisMatch<size_t> if there is a size mismatch.
@return Matrix(This * A)
*/
{
  if (m_numColumns != A.m_numRows)
    throw Kernel::Exception::MisMatch<size_t>(m_numColumns, A.m_numRows, "Matrix::operator*(Matrix)");
  Matrix<T> X(m_numRows, A.m_numColumns);
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < A.m_numColumns; j++) {
      for (size_t kk = 0; kk < m_numColumns; kk++) {
        X.m_rawData[i][j] += m_rawData[i][kk] * A.m_rawData[kk][j];
      }
    }
  }
  return X;
}

template <typename T>
std::vector<T> Matrix<T>::operator*(const std::vector<T> &Vec) const
/**
Matrix multiplication THIS * Vec to produce a vec
@param Vec :: size of vector > this->nrows
@throw MisMatch<size_t> if there is a size mismatch.
@return Matrix(This * Vec)
*/
{
  if (m_numColumns > Vec.size())
    throw Kernel::Exception::MisMatch<size_t>(m_numColumns, Vec.size(), "Matrix::operator*(m_rawDataec)");

  std::vector<T> Out(m_numRows);
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      Out[i] += m_rawData[i][j] * Vec[j];
    }
  }
  return Out;
}

/**
Matrix multiplication THIS * Vec to produce a vec
@param in :: size of vector > this->nrows
@param out :: result of Matrix(This * Vec)
@throw MisMatch<size_t> if there is a size mismatch.
*/
template <typename T> void Matrix<T>::multiplyPoint(const std::vector<T> &in, std::vector<T> &out) const {
  out.resize(m_numRows);
  std::fill(std::begin(out), std::end(out), static_cast<T>(0.0));
  if (m_numColumns > in.size())
    throw Kernel::Exception::MisMatch<size_t>(m_numColumns, in.size(), "Matrix::multiplyPoint(in,out)");
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      out[i] += m_rawData[i][j] * in[j];
    }
  }
}

template <typename T>
V3D Matrix<T>::operator*(const V3D &Vx) const
/**
Matrix multiplication THIS * V
@param Vx :: Colunm vector to multiply by
@throw MisMatch<size_t> if there is a size mismatch.
@return Matrix(This * A)
*/
{
  if (m_numColumns != 3 || m_numRows > 3)
    throw Kernel::Exception::MisMatch<size_t>(m_numColumns, 3, "Matrix::operator*(m_rawData3D)");

  V3D v;
  for (size_t i = 0; i < m_numRows; ++i) {
    v[i] = m_rawData[i][0] * Vx.X() + m_rawData[i][1] * Vx.Y() + m_rawData[i][2] * Vx.Z();
  }

  return v;
}

template <typename T>
Matrix<T> Matrix<T>::operator*(const T &Value) const
/**
Matrix multiplication THIS * Value
@param Value :: Scalar to multiply by
@return V * (this)
*/
{
  Matrix<T> X(*this);
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      X.m_rawData[i][j] *= Value;
    }
  }
  return X;
}

/**
Matrix multiplication THIS *= A
Note that we call operator* to avoid the problem
of changing matrix size.
@param A :: Matrix to multiply by  (this->row must == A->columns)
@return This *= A
*/
template <typename T> Matrix<T> &Matrix<T>::operator*=(const Matrix<T> &A) {
  if (m_numColumns != A.m_numRows)
    throw Kernel::Exception::MisMatch<size_t>(m_numColumns, A.m_numRows, "Matrix*=(Matrix<T>)");
  // This construct to avoid the problem of changing size
  *this = this->operator*(A);
  return *this;
}

template <typename T>
Matrix<T> &Matrix<T>::operator*=(const T &Value)
/**
Matrix multiplication THIS * Value
@param Value :: Scalar to multiply matrix by
@return *this
*/
{
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      m_rawData[i][j] *= Value;
    }
  }
  return *this;
}

template <typename T>
Matrix<T> &Matrix<T>::operator/=(const T &Value)
/**
Matrix divishio THIS / Value
@param Value :: Scalar to multiply matrix by
@return *this
*/
{
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      m_rawData[i][j] /= Value;
    }
  }
  return *this;
}

template <typename T>
bool Matrix<T>::operator!=(const Matrix<T> &A) const
/**
Element by Element comparison
@param A :: Matrix to check
@return true :: on succees
@return false :: failure
*/
{
  return !(this->operator==(A));
}

template <typename T>
bool Matrix<T>::operator==(const Matrix<T> &A) const
/**
Element by element comparison within tolerance.
Tolerance means that the value must be > tolerance
and less than (diff/max)>tolerance
Always returns 0 if the Matrix have different sizes
@param A :: matrix to check.
@return true on success
*/
{
  return this->equals(A, 1e-8);
}

//---------------------------------------------------------------------------------------
template <typename T>
bool Matrix<T>::equals(const Matrix<T> &A, const double Tolerance) const
/**
Element by element comparison within tolerance.
Tolerance means that the value must be > tolerance
and less than (diff/max)>tolerance
Always returns 0 if the Matrix have different sizes
@param A :: matrix to check.
@param Tolerance :: tolerance in comparing elements
@return true on success
*/
{
  if (&A != this) // this == A == always true
  {
    if (A.m_numRows != m_numRows || A.m_numColumns != m_numColumns)
      return false;

    double maxS(0.0);
    double maxDiff(0.0); // max di
    for (size_t i = 0; i < m_numRows; i++)
      for (size_t j = 0; j < m_numColumns; j++) {
        const T diff = (m_rawData[i][j] - A.m_rawData[i][j]);
        if (std::isnan(static_cast<double>(diff))) {
          maxDiff = std::numeric_limits<T>::max();
        } else {
          if (fabs(diff) > maxDiff)
            maxDiff = fabs(diff);
        }
        if (fabs(m_rawData[i][j]) > maxS)
          maxS = fabs(m_rawData[i][j]);
      }
    if (maxDiff < Tolerance)
      return true;
    if (maxS > 1.0 && (maxDiff / maxS) < Tolerance)
      return true;
    return false;
  }
  // this == this is true
  return true;
}

//---------------------------------------------------------------------------------------
/** Element by element comparison of <
Always returns false if the Matrix have different sizes
@param A :: matrix to check.
@return true if this < A
*/
template <typename T> bool Matrix<T>::operator<(const Matrix<T> &A) const {
  if (&A == this) // this < A == always false
    return false;

  if (A.m_numRows != m_numRows || A.m_numColumns != m_numColumns)
    return false;

  for (size_t i = 0; i < m_numRows; i++)
    for (size_t j = 0; j < m_numColumns; j++) {
      if (m_rawData[i][j] >= A.m_rawData[i][j])
        return false;
    }
  return true;
}

//---------------------------------------------------------------------------------------
/** Element by element comparison of >=
Always returns false if the Matrix have different sizes
@param A :: matrix to check.
@return true if this >= A
*/
template <typename T> bool Matrix<T>::operator>=(const Matrix<T> &A) const {
  if (&A == this)
    return true;

  if (A.m_numRows != m_numRows || A.m_numColumns != m_numColumns)
    return false;

  for (size_t i = 0; i < m_numRows; i++)
    for (size_t j = 0; j < m_numColumns; j++) {
      if (m_rawData[i][j] < A.m_rawData[i][j])
        return false;
    }
  return true;
}

/**
Sets the memory held in matrix
@param a :: number of rows
@param b :: number of columns
*/
template <typename T> void Matrix<T>::setMem(const size_t a, const size_t b) {
  if (a == m_numRows && b == m_numColumns && m_rawData != nullptr)
    return;

  if (a == 0 || b == 0)
    return;

  m_numRows = a;
  m_numColumns = b;

  // Allocate memory first - this has to be a flat 1d array masquerading as a 2d
  // array so we can expose the memory to Python APIs via numpy which expects
  // this
  // style of memory layout.
  auto allocatedMemory = std::make_unique<T[]>((m_numRows * m_numColumns));

  // Next allocate an array of pointers for the rows (X). This partitions
  // the 1D array into a 2D array for callers.
  auto rowPtrs = std::make_unique<T *[]>(m_numRows);

  for (size_t i = 0; i < m_numRows; i++) {
    // Calculate offsets into the allocated memory array (Y)
    rowPtrs[i] = &allocatedMemory[i * m_numColumns];
  }

  m_rawDataAlloc = std::move(allocatedMemory);
  m_rawData = std::move(rowPtrs);
}

/**
Swap rows I and J
@param RowI :: row I to swap
@param RowJ :: row J to swap
*/
template <typename T> void Matrix<T>::swapRows(const size_t RowI, const size_t RowJ) {
  if ((m_numRows * m_numColumns > 0) && RowI < m_numRows && RowJ < m_numRows && RowI != RowJ) {
    for (size_t k = 0; k < m_numColumns; k++) {
      T tmp = m_rawData[RowI][k];
      m_rawData[RowI][k] = m_rawData[RowJ][k];
      m_rawData[RowJ][k] = tmp;
    }
  }
}

/**
Swap columns I and J
@param colI :: col I to swap
@param colJ :: col J to swap
*/
template <typename T> void Matrix<T>::swapCols(const size_t colI, const size_t colJ) {
  if ((m_numRows * m_numColumns) > 0 && colI < m_numColumns && colJ < m_numColumns && colI != colJ) {
    for (size_t k = 0; k < m_numRows; k++) {
      T tmp = m_rawData[k][colI];
      m_rawData[k][colI] = m_rawData[k][colJ];
      m_rawData[k][colJ] = tmp;
    }
  }
}

template <typename T>
void Matrix<T>::zeroMatrix()
/**
Zeros all elements of the matrix
*/
{
  if ((m_numRows * m_numColumns) > 0) {
    for (size_t i = 0; i < m_numRows; i++) {
      for (size_t j = 0; j < m_numColumns; j++) {
        m_rawData[i][j] = static_cast<T>(0);
      }
    }
  }
}

template <typename T>
void Matrix<T>::identityMatrix()
/**
Makes the matrix an idenity matrix.
Zeros all the terms outside of the square
*/
{
  if ((m_numRows * m_numColumns) > 0) {
    for (size_t i = 0; i < m_numRows; i++) {
      for (size_t j = 0; j < m_numColumns; j++) {
        m_rawData[i][j] = static_cast<T>(j == i);
      }
    }
  }
}
template <typename T> void Matrix<T>::setColumn(const size_t nCol, const std::vector<T> &newCol) {
  if (nCol >= this->m_numColumns) {
    throw(std::invalid_argument("nCol requested> nCol availible"));
  }
  size_t m_numRowsM = newCol.size();
  if (m_numRows < m_numRowsM)
    m_numRowsM = m_numRows;
  for (size_t i = 0; i < m_numRowsM; i++) {
    m_rawData[i][nCol] = newCol[i];
  }
}
template <typename T> void Matrix<T>::setRow(const size_t nRow, const std::vector<T> &newRow) {
  if (nRow >= this->m_numRows) {
    throw(std::invalid_argument("nRow requested> nRow availible"));
  }
  size_t m_numColumnsM = newRow.size();
  if (m_numColumns < m_numColumnsM)
    m_numColumnsM = m_numColumns;
  for (size_t j = 0; j < m_numColumnsM; j++) {
    m_rawData[nRow][j] = newRow[j];
  }
}

template <typename T>
void Matrix<T>::rotate(const double tau, const double s, const int i, const int j, const int k, const int m)
/**
Applies a rotation to a particular point of tan(theta)=tau.
Note that you need both sin(theta) and tan(theta) because of
sign preservation.
@param tau :: tan(theta)
@param s :: sin(theta)
@param i ::  first index (xpos)
@param j ::  first index (ypos)
@param k ::  second index (xpos)
@param m ::  second index (ypos)
*/
{
  const T gg = m_rawData[i][j];
  const T hh = m_rawData[k][m];
  m_rawData[i][j] = static_cast<T>(gg - s * (hh + gg * tau));
  m_rawData[k][m] = static_cast<T>(hh + s * (gg - hh * tau));
}

template <typename T>
Matrix<T> Matrix<T>::preMultiplyByDiagonal(const std::vector<T> &Dvec) const
/**
Creates a diagonal matrix D from the given vector Dvec and
PRE-multiplies the matrix by it (i.e. D * M).
@param Dvec :: diagonal matrix (just centre points)
@return D*this
*/
{
  if (Dvec.size() != m_numRows) {
    std::ostringstream cx;
    cx << "Matrix::preMultiplyByDiagonal Size: " << Dvec.size() << " " << m_numRows << " " << m_numColumns;
    throw std::runtime_error(cx.str());
  }
  Matrix<T> X(Dvec.size(), m_numColumns);
  for (size_t i = 0; i < Dvec.size(); i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      X.m_rawData[i][j] = Dvec[i] * m_rawData[i][j];
    }
  }
  return X;
}

template <typename T>
Matrix<T> Matrix<T>::postMultiplyByDiagonal(const std::vector<T> &Dvec) const
/**
Creates a diagonal matrix D from the given vector Dvec and
POST-multiplies the matrix by it (i.e. M * D).
@param Dvec :: diagonal matrix (just centre points)
@return this*D
*/
{
  if (Dvec.size() != m_numColumns) {
    std::ostringstream cx;
    cx << "Error Matrix::bDiaognal size:: " << Dvec.size() << " " << m_numRows << " " << m_numColumns;
    throw std::runtime_error(cx.str());
  }

  Matrix<T> X(m_numRows, Dvec.size());
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < Dvec.size(); j++) {
      X.m_rawData[i][j] = Dvec[j] * m_rawData[i][j];
    }
  }
  return X;
}

template <typename T>
Matrix<T> Matrix<T>::Tprime() const
/**
Transpose the matrix :
Has transpose for a square matrix case.
@return M^T
*/
{
  if ((m_numRows * m_numColumns) == 0)
    return *this;

  if (m_numRows == m_numColumns) // inplace transpose
  {
    Matrix<T> MT(*this);
    MT.Transpose();
    return MT;
  }

  // irregular matrix
  Matrix<T> MT(m_numColumns, m_numRows);
  for (size_t i = 0; i < m_numRows; i++)
    for (size_t j = 0; j < m_numColumns; j++)
      MT.m_rawData[j][i] = m_rawData[i][j];

  return MT;
}

template <typename T>
Matrix<T> &Matrix<T>::Transpose()
/**
Transpose the matrix :
Has a in place transpose for a square matrix case.
@return this^T
*/
{
  if ((m_numRows * m_numColumns) == 0)
    return *this;

  if (m_numRows == m_numColumns) // in place transpose
  {
    for (size_t i = 0; i < m_numRows; i++) {
      for (size_t j = i + 1; j < m_numColumns; j++) {
        std::swap(m_rawData[i][j], m_rawData[j][i]);
      }
    }
    return *this;
  }

  // irregular matrix
  // get some memory

  auto allocatedMemory = std::make_unique<T[]>((m_numRows * m_numColumns));
  auto transposePtrs = std::make_unique<T *[]>(m_numRows);

  for (size_t i = 0; i < m_numColumns; i++) {
    // Notice how this partitions using Rows (X) instead of Cols(Y)
    transposePtrs[i] = &allocatedMemory[i * m_numRows];
  }

  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      transposePtrs[j][i] = m_rawData[i][j];
    }
  }
  // remove old memory
  std::swap(m_numRows, m_numColumns);

  m_rawDataAlloc = std::move(allocatedMemory);
  m_rawData = std::move(transposePtrs);

  return *this;
}
template <typename T>
void Matrix<T>::GaussJordan(Matrix<T> &B)
/**
Invert this matrix in place using Gauss-Jordan elimination.
Matrix will be replaced by its inverse.
@param B :: [input, output] Must have same dimensions as A. Returned as
identity matrix. (?)
@throw std::invalid_argument on input error
@throw std::runtime_error if singular
*/
{
  // check for input errors
  if (m_numRows != m_numColumns || B.m_numRows != m_numRows) {
    throw std::invalid_argument("Matrix not square, or sizes do not match");
  }

  // pivoted rows
  std::vector<int> pivoted(m_numRows);
  fill(pivoted.begin(), pivoted.end(), 0);

  std::vector<int> indxcol(m_numRows); // Column index
  std::vector<int> indxrow(m_numRows); // row index

  size_t irow(0), icol(0);
  for (size_t i = 0; i < m_numRows; i++) {
    // Get Biggest non-pivoted item
    double bigItem = 0.0; // get point to pivot over
    for (size_t j = 0; j < m_numRows; j++) {
      if (pivoted[j] != 1) // check only non-pivots
      {
        for (size_t k = 0; k < m_numRows; k++) {
          if (!pivoted[k]) {
            if (fabs(m_rawData[j][k]) >= bigItem) {
              bigItem = fabs(m_rawData[j][k]);
              irow = j;
              icol = k;
            }
          }
        }
      }
    }
    pivoted[icol]++;
    // Swap in row/col to make a diagonal item
    if (irow != icol) {
      swapRows(irow, icol);
      B.swapRows(irow, icol);
    }
    indxrow[i] = static_cast<int>(irow);
    indxcol[i] = static_cast<int>(icol);

    if (m_rawData[icol][icol] == 0.0) {
      throw std::runtime_error("Error doing G-J elem on a singular matrix");
    }
    const T pivDiv = T(1.0) / m_rawData[icol][icol];
    m_rawData[icol][icol] = 1;
    for (size_t l = 0; l < m_numRows; l++) {
      m_rawData[icol][l] *= pivDiv;
    }
    for (size_t l = 0; l < B.m_numColumns; l++) {
      B.m_rawData[icol][l] *= pivDiv;
    }

    for (size_t ll = 0; ll < m_numRows; ll++) {
      if (ll != icol) {
        const T div_num = m_rawData[ll][icol];
        m_rawData[ll][icol] = 0.0;
        for (size_t l = 0; l < m_numRows; l++) {
          m_rawData[ll][l] -= m_rawData[icol][l] * div_num;
        }
        for (size_t l = 0; l < B.m_numColumns; l++) {
          B.m_rawData[ll][l] -= B.m_rawData[icol][l] * div_num;
        }
      }
    }
  }

  // Un-roll interchanges
  if (m_numRows > 0) {
    for (int l = static_cast<int>(m_numRows) - 1; l >= 0; l--) {
      if (indxrow[l] != indxcol[l]) {
        swapCols(indxrow[l], indxcol[l]);
      }
    }
  }
}

template <typename T>
T Matrix<T>::Invert()
/**
If the Matrix is square then invert the matrix
using LU decomposition
@return Determinant (0 if the matrix is singular)
*/
{
  if (m_numRows != m_numColumns && m_numRows < 1)
    return 0;

  if (m_numRows == 1) {
    T det = m_rawData[0][0];
    if (m_rawData[0][0] != static_cast<T>(0.))
      m_rawData[0][0] = static_cast<T>(1.) / m_rawData[0][0];
    return det;
  }
  std::vector<int> indx(m_numRows); // Set in lubcmp
  std::vector<double> col(m_numRows);

  int determinantInterchange = 0;
  Matrix<T> Lcomp(*this);
  Lcomp.lubcmp(indx.data(), determinantInterchange);

  auto det = static_cast<double>(determinantInterchange);
  for (size_t j = 0; j < m_numRows; j++)
    det *= Lcomp.m_rawData[j][j];

  for (size_t j = 0; j < m_numRows; j++) {
    for (size_t i = 0; i < m_numRows; i++)
      col[i] = 0.0;
    col[j] = 1.0;
    Lcomp.lubksb(indx.data(), col.data());
    for (size_t i = 0; i < m_numRows; i++)
      m_rawData[i][j] = static_cast<T>(col[i]);
  }
  return static_cast<T>(det);
}

template <typename T>
void Matrix<T>::invertTridiagonal(double tolerance)
/**
Check it's a square tridiagonal matrix with all diagonal elements equal and if
yes invert the matrix using analytic formula. If not then use standard Invert
*/
{
  bool regular = true;
  if ((numRows() > 1) && (numCols() > 1)) {
    if (numRows() == numCols()) {
      std::vector<T> diagonal = {m_rawData[0][0], m_rawData[1][0]};
      for (size_t i = 1; i < numRows() && regular; i++) {
        for (size_t j = 1; i < numCols() && regular; i++) {
          size_t diff = std::abs(static_cast<int>(i - j));
          if (diff < 2) {
            if (std::abs(diagonal[diff] - m_rawData[i][j]) > tolerance) {
              regular = false;
            }
          } else if (m_rawData[i][j] != 0) {
            throw std::runtime_error("Matrix is not tridiagonal");
          }
        }
      }
    } else {
      regular = false;
    }
  }
  if (regular) {
    // use analytic expression as described in G Y Hu and R F O'Connell (1996)
    T scalefactor = numRows() > 1 ? m_rawData[1][0] : 1;
    *this /= scalefactor;
    long double D = m_rawData[0][0];
    long double k = static_cast<long double>(numRows());

    for (size_t i = 0; i < numRows(); i++) {
      for (size_t j = 0; j < numCols(); j++) {
        long double lambda;
        auto iMinusj = std::abs(static_cast<long double>(i) - static_cast<long double>(j));
        auto iPlusj = static_cast<long double>(i + j);
        if (D >= 2) {
          m_rawData[i][j] = static_cast<T>(std::pow(-1.0, iPlusj));
          lambda = std::acosh(D / 2.0);
        } else if (D > -2.0) {
          m_rawData[i][j] = 1;          // use +1 here instead of the -1 in the paper
          lambda = std::acos(-D / 2.0); // extra minus sign here compared to paper
        } else {
          m_rawData[i][j] = -1;
          lambda = std::acosh(-D / 2.0);
        }
        if (std::abs(D) > 2.0) {
          long double a, b, c;
          if (((k + 1) * std::asinh(D / 2.0)) <= std::asinh(std::numeric_limits<long double>::max())) {
            a = std::cosh((k + 1 - iMinusj) * lambda);
            b = std::cosh((k + 1 - iPlusj - 2) * lambda); // extra -2 because i and j are 1-based in the paper
            c = static_cast<long double>(2.0) * std::sinh(lambda) * std::sinh((k + 1) * lambda);
          } else {
            // cosh, sinh overflow for x~>710 so use approximation based on expansion of cosh, sinh into e^x and e^-x
            a = std::exp(-iMinusj * lambda);
            b = std::exp(-(iPlusj + 2) * lambda) + std::exp(-(2 * k - iPlusj) * lambda);
            c = static_cast<long double>(2.0) * std::sinh(lambda);
          }

          long double value = m_rawData[i][j];
          value *= (a - b) / c;
          m_rawData[i][j] = static_cast<T>(value);
        } else if (std::abs(D) == 2.0) {
          long double value = m_rawData[i][j];
          value *= (2 * k + 2 - iMinusj - iPlusj - 2) * (iPlusj + 2 - iMinusj);
          value /= (4 * (k + 1));
          m_rawData[i][j] = static_cast<T>(value);
        } else {
          long double value = m_rawData[i][j];
          value *= std::cos((k + 1 - iMinusj) * lambda) -
                   std::cos((k + 1 - iPlusj - 2) * lambda); // extra -2 because i and j are 1-based in the paper
          value /= 2 * std::sin(lambda) * std::sin((k + 1) * lambda);
          m_rawData[i][j] = static_cast<T>(value);
        }
      }
    }
    *this /= scalefactor;
  } else {
    Invert();
  }
}

template <typename T>
T Matrix<T>::determinant() const
/**
Calculate the derminant of the matrix
@return Determinant of matrix.
*/
{
  if (m_numRows != m_numColumns)
    throw Kernel::Exception::MisMatch<size_t>(m_numRows, m_numColumns, "Determinant error :: Matrix is not square");

  Matrix<T> Mt(*this); // temp copy
  T D = Mt.factor();
  return D;
}

template <typename T>
T Matrix<T>::factor()
/**
Gauss jordan diagonal factorisation
The diagonal is left as the values,
the lower part is zero.
@return the factored matrix
*/
{
  if (m_numRows != m_numColumns || m_numRows < 1)
    throw std::runtime_error("Matrix::factor Matrix is not square");

  double deter = 1.0;
  for (int i = 0; i < static_cast<int>(m_numRows) - 1; i++) // loop over each row
  {
    int jmax = i;
    double Pmax = fabs(m_rawData[i][i]);
    for (int j = i + 1; j < static_cast<int>(m_numRows); j++) // find max in Row i
    {
      if (fabs(m_rawData[i][j]) > Pmax) {
        Pmax = fabs(m_rawData[i][j]);
        jmax = j;
      }
    }
    if (Pmax < 1e-8) // maxtrix signular
    {
      //          std::cerr<<"Matrix Singular"<<'\n';
      return 0;
    }
    // Swap Columns
    if (i != jmax) {
      swapCols(i, jmax);
      deter *= -1; // change sign.
    }
    // zero all rows below diagonal
    Pmax = m_rawData[i][i];
    deter *= Pmax;
    for (int k = i + 1; k < static_cast<int>(m_numRows); k++) // row index
    {
      const double scale = m_rawData[k][i] / Pmax;
      m_rawData[k][i] = static_cast<T>(0);
      for (int q = i + 1; q < static_cast<int>(m_numRows); q++) // column index
        m_rawData[k][q] -= static_cast<T>(scale * m_rawData[i][q]);
    }
  }
  deter *= m_rawData[m_numRows - 1][m_numRows - 1];
  return static_cast<T>(deter);
}

template <typename T>
void Matrix<T>::normVert()
/**
Normalise EigenVectors
Assumes that they have already been calculated
*/
{
  for (size_t i = 0; i < m_numRows; i++) {
    T sum = 0;
    for (size_t j = 0; j < m_numColumns; j++) {
      sum += m_rawData[i][j] * m_rawData[i][j];
    }
    sum = static_cast<T>(std::sqrt(static_cast<double>(sum)));
    for (size_t j = 0; j < m_numColumns; j++) {
      m_rawData[i][j] /= sum;
    }
  }
}

template <typename T>
T Matrix<T>::compSum() const
/**
Add up each component sums for the matrix
@return \f$ \sum_i \sum_j V_{ij}^2 \f$
*/
{
  T sum(0);
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      sum += m_rawData[i][j] * m_rawData[i][j];
    }
  }
  return sum;
}

template <typename T>
void Matrix<T>::lubcmp(int *rowperm, int &interchange)
/**
Find biggest pivot and move to top row. Then
divide by pivot.
@param interchange :: odd/even nterchange (+/-1)
@param rowperm :: row permutations [m_numRows values]
*/
{
  double sum, dum, big, temp;

  if (m_numRows != m_numColumns || m_numRows < 2) {
    std::cerr << "Error with lubcmp\n";
    return;
  }
  std::vector<double> result(m_numRows);
  interchange = 1;
  for (int i = 0; i < static_cast<int>(m_numRows); i++) {
    big = 0.0;
    for (int j = 0; j < static_cast<int>(m_numRows); j++)
      if ((temp = fabs(m_rawData[i][j])) > big)
        big = temp;

    if (big == 0.0) {
      for (int j = 0; j < static_cast<int>(m_numRows); j++) {
        rowperm[j] = j;
      }
      return;
    }
    result[i] = 1.0 / big;
  }

  for (int j = 0; j < static_cast<int>(m_numRows); j++) {
    for (int i = 0; i < j; i++) {
      sum = m_rawData[i][j];
      for (int k = 0; k < i; k++)
        sum -= m_rawData[i][k] * m_rawData[k][j];
      m_rawData[i][j] = static_cast<T>(sum);
    }
    big = 0.0;
    int imax = j;
    for (int i = j; i < static_cast<int>(m_numRows); i++) {
      sum = m_rawData[i][j];
      for (int k = 0; k < j; k++)
        sum -= m_rawData[i][k] * m_rawData[k][j];
      m_rawData[i][j] = static_cast<T>(sum);
      if ((dum = result[i] * fabs(sum)) >= big) {
        big = dum;
        imax = i;
      }
    }

    if (j != imax) {
      for (int k = 0; k < static_cast<int>(m_numRows); k++) { // Interchange rows
        dum = m_rawData[imax][k];
        m_rawData[imax][k] = m_rawData[j][k];
        m_rawData[j][k] = static_cast<T>(dum);
      }
      interchange *= -1;
      result[imax] = result[j];
    }
    rowperm[j] = imax;

    if (m_rawData[j][j] == 0.0)
      m_rawData[j][j] = static_cast<T>(1e-14);
    if (j != static_cast<int>(m_numRows) - 1) {
      dum = 1.0 / (m_rawData[j][j]);
      for (int i = j + 1; i < static_cast<int>(m_numRows); i++)
        m_rawData[i][j] *= static_cast<T>(dum);
    }
  }
}

template <typename T>
void Matrix<T>::lubksb(const int *rowperm, double *b)
/**
Implements a separation of the Matrix
into a triangular matrix
*/
{
  int ii = -1;

  for (int i = 0; i < static_cast<int>(m_numRows); i++) {
    int ip = rowperm[i];
    double sum = b[ip];
    b[ip] = b[i];
    if (ii != -1)
      for (int j = ii; j < i; j++)
        sum -= m_rawData[i][j] * b[j];
    else if (sum != 0.)
      ii = i;
    b[i] = sum;
  }

  for (int i = static_cast<int>(m_numRows) - 1; i >= 0; i--) {
    double sum = b[i];
    for (int j = i + 1; j < static_cast<int>(m_numRows); j++)
      sum -= m_rawData[i][j] * b[j];
    b[i] = sum / m_rawData[i][i];
  }
}

template <typename T>
void Matrix<T>::averSymmetric()
/**
Simple function to create an average symmetric matrix
out of the Matrix
*/
{
  const size_t minSize = (m_numRows > m_numColumns) ? m_numColumns : m_numRows;
  for (size_t i = 0; i < minSize; i++) {
    for (size_t j = i + 1; j < minSize; j++) {
      m_rawData[i][j] = (m_rawData[i][j] + m_rawData[j][i]) / 2;
      m_rawData[j][i] = m_rawData[i][j];
    }
  }
}

template <typename T>
std::vector<T> Matrix<T>::Diagonal() const
/**
Returns the diagonal form as a vector
@return Diagonal elements
*/
{
  const size_t Msize = (m_numColumns > m_numRows) ? m_numRows : m_numColumns;
  std::vector<T> Diag(Msize);
  for (size_t i = 0; i < Msize; i++) {
    Diag[i] = m_rawData[i][i];
  }
  return Diag;
}

template <typename T>
T Matrix<T>::Trace() const
/**
Calculates the trace of the matrix
@return Trace of matrix
*/
{
  const size_t Msize = (m_numColumns > m_numRows) ? m_numRows : m_numColumns;
  T Trx = 0;
  for (size_t i = 0; i < Msize; i++) {
    Trx += m_rawData[i][i];
  }
  return Trx;
}

template <typename T>
void Matrix<T>::sortEigen(Matrix<T> &DiagMatrix)
/**
Sorts the eigenvalues into increasing
size. Moves the EigenVectors correspondingly
@param DiagMatrix :: matrix of the EigenValues
*/
{
  if (m_numColumns != m_numRows || m_numRows != DiagMatrix.m_numRows || m_numRows != DiagMatrix.m_numColumns) {
    std::cerr << "Matrix not Eigen Form\n";
    throw(std::invalid_argument(" Matrix is not in an eigenvalue format"));
  }
  std::vector<int> index;
  std::vector<T> X = DiagMatrix.Diagonal();
  indexSort(X, index);
  Matrix<T> EigenVec(*this);
  for (size_t Icol = 0; Icol < m_numRows; Icol++) {
    for (size_t j = 0; j < m_numRows; j++) {
      m_rawData[j][Icol] = EigenVec[j][index[Icol]];
    }
    DiagMatrix[Icol][Icol] = X[index[Icol]];
  }
}

template <typename T>
int Matrix<T>::Diagonalise(Matrix<T> &EigenVec, Matrix<T> &DiagMatrix) const
/**
Attempt to diagonalise the matrix IF symmetric
@param EigenVec :: (output) the Eigenvectors matrix
@param DiagMatrix :: the diagonal matrix of eigenvalues
@return :: 1  on success 0 on failure
*/
{
  if (m_numRows != m_numColumns || m_numRows < 1) {
    std::cerr << "Matrix not square\n";
    return 0;
  }
  for (size_t i = 0; i < m_numRows; i++)
    for (size_t j = i + 1; j < m_numRows; j++)
      if (fabs(m_rawData[i][j] - m_rawData[j][i]) > 1e-6) {
        std::cerr << "Matrix not symmetric\n";
        std::cerr << (*this);
        return 0;
      }

  Matrix<T> A(*this);
  // Make V an identity matrix
  EigenVec.setMem(m_numRows, m_numRows);
  EigenVec.identityMatrix();
  DiagMatrix.setMem(m_numRows, m_numRows);
  DiagMatrix.zeroMatrix();

  std::vector<double> Diag(m_numRows);
  std::vector<double> B(m_numRows);
  std::vector<double> ZeroComp(m_numRows);
  // set b and d to the diagonal elements o A
  for (size_t i = 0; i < m_numRows; i++) {
    Diag[i] = B[i] = A.m_rawData[i][i];
    ZeroComp[i] = 0;
  }

  for (int i = 0; i < 100; i++) // max 50 iterations
  {
    double sm = 0.0; // sum of off-diagonal terms
    for (size_t ip = 0; ip < m_numRows - 1; ip++)
      for (size_t iq = ip + 1; iq < m_numRows; iq++)
        sm += fabs(A.m_rawData[ip][iq]);

    if (sm == 0.0) // Nothing to do return...
    {
      // Make OUTPUT -- D + A
      // sort Output::
      for (size_t ix = 0; ix < m_numRows; ix++)
        DiagMatrix.m_rawData[ix][ix] = static_cast<T>(Diag[ix]);
      return 1;
    }

    // Threshold large for first 5 sweeps
    double tresh = (i < 6) ? 0.2 * sm / static_cast<int>(m_numRows * m_numRows) : 0.0;

    for (int ip = 0; ip < static_cast<int>(m_numRows) - 1; ip++) {
      for (int iq = ip + 1; iq < static_cast<int>(m_numRows); iq++) {
        double g = 100.0 * fabs(A.m_rawData[ip][iq]);
        // After 4 sweeps skip if off diagonal small
        if (i > 6 && static_cast<float>(fabs(Diag[ip] + g)) == static_cast<float>(fabs(Diag[ip])) &&
            static_cast<float>(fabs(Diag[iq] + g)) == static_cast<float>(fabs(Diag[iq])))
          A.m_rawData[ip][iq] = 0;

        else if (fabs(A.m_rawData[ip][iq]) > tresh) {
          double tanAngle, cosAngle, sinAngle;
          double h = Diag[iq] - Diag[ip];
          double cot2Theta = 0.5 * h / A.m_rawData[ip][iq];
          // tanAngle formula well behaved even if cot2Theta is inf
          tanAngle = 1.0 / (fabs(cot2Theta) + sqrt(1.0 + pow(cot2Theta, 2)));
          if (cot2Theta < 0.0)
            tanAngle = -tanAngle;
          cosAngle = 1.0 / sqrt(1 + tanAngle * tanAngle);
          sinAngle = tanAngle * cosAngle;
          double tau = sinAngle / (1.0 + cosAngle);
          h = tanAngle * A.m_rawData[ip][iq];
          ZeroComp[ip] -= h;
          ZeroComp[iq] += h;
          Diag[ip] -= h;
          Diag[iq] += h;
          A.m_rawData[ip][iq] = 0;
          // Rotations 0<j<p
          for (int j = 0; j < ip; j++)
            A.rotate(tau, sinAngle, j, ip, j, iq);
          for (int j = ip + 1; j < iq; j++)
            A.rotate(tau, sinAngle, ip, j, j, iq);
          for (int j = iq + 1; j < static_cast<int>(m_numRows); j++)
            A.rotate(tau, sinAngle, ip, j, iq, j);
          for (int j = 0; j < static_cast<int>(m_numRows); j++)
            EigenVec.rotate(tau, sinAngle, j, ip, j, iq);
        }
      }
    }
    for (size_t j = 0; j < m_numRows; j++) {
      B[j] += ZeroComp[j];
      Diag[j] = B[j];
      ZeroComp[j] = 0.0;
    }
  }
  std::cerr << "Error :: Iterations are a problem\n";
  return 0;
}

template <typename T>
bool Matrix<T>::isRotation() const
/** Check if a matrix represents a proper rotation
@ return :: true/false
*/
{
  if (this->m_numRows != this->m_numColumns)
    throw(std::invalid_argument("matrix is not square"));
  //  std::cout << "Matrix determinant-1 is " << (this->determinant()-1) <<
  //  '\n';
  if (fabs(this->determinant() - 1) > 1e-5) {
    return false;
  } else {
    Matrix<T> prod(m_numRows, m_numColumns), ident(m_numRows, m_numColumns, true);
    prod = this->operator*(this->Tprime());
    //    std::cout << "Matrix * Matrix' = " << std::endl << prod << '\n';
    return prod.equals(ident, 1e-5);
  }
}

template <typename T>
bool Matrix<T>::isOrthogonal() const
/** Check if a matrix is orthogonal. Same as isRotation, but allows determinant
to be -1
@ return :: true/false
*/
{
  if (this->m_numRows != this->m_numColumns)
    throw(std::invalid_argument("matrix is not square"));
  if (fabs(fabs(this->determinant()) - 1.) > 1e-5) {
    return false;
  } else {
    Matrix<T> prod(m_numRows, m_numColumns), ident(m_numRows, m_numColumns, true);
    prod = this->operator*(this->Tprime());
    return prod.equals(ident, 1e-7);
  }
}

template <typename T>
std::vector<T> Matrix<T>::toRotation()
/**
Transform the matrix to a rotation matrix, by normalizing each column to 1
@return :: a vector of scaling factors
@throw :: std::invalid_argument if the absolute value of the determinant is
less then 1e-10 or not square matrix
*/
{
  if (this->m_numRows != this->m_numColumns)
    throw(std::invalid_argument("matrix is not square"));
  if (fabs(this->determinant()) < 1e-10)
    throw(std::invalid_argument("Determinant is too small"));
  // step 1: orthogonalize the matrix
  for (size_t i = 0; i < this->m_numColumns; ++i) {
    double spself = 0.;
    for (size_t j = 0; j < this->m_numRows; ++j)
      spself += (m_rawData[j][i] * m_rawData[j][i]);
    for (size_t k = i + 1; k < this->m_numColumns; ++k) {
      double spother = 0;
      for (size_t j = 0; j < this->m_numRows; ++j)
        spother += (m_rawData[j][i] * m_rawData[j][k]);
      for (size_t j = 0; j < this->m_numRows; ++j)
        m_rawData[j][k] -= static_cast<T>(m_rawData[j][i] * spother / spself);
    }
  }
  // step 2: get scales and rescsale the matrix
  std::vector<T> scale(this->m_numRows);
  for (size_t i = 0; i < this->m_numColumns; ++i) {
    T currentScale{0};
    for (size_t j = 0; j < this->m_numRows; ++j)
      currentScale += (m_rawData[j][i] * m_rawData[j][i]);
    currentScale = static_cast<T>(sqrt(static_cast<double>(currentScale)));
    if (currentScale < 1e-10)
      throw(std::invalid_argument("Scale is too small"));
    scale[i] = currentScale;
  }
  Matrix<T> scalingMatrix(m_numRows, m_numColumns), change(m_numRows, m_numColumns, true);
  for (size_t i = 0; i < this->m_numColumns; ++i)
    scalingMatrix[i][i] = static_cast<T>(1.0 / scale[i]);
  *this = this->operator*(scalingMatrix);
  if (this->determinant() < 0.) {
    scale[0] = -scale[0];
    change[0][0] = static_cast<T>(-1);
    *this = this->operator*(change);
  }
  return scale;
}

template <typename T>
void Matrix<T>::print() const
/**
Simple print out routine
*/
{
  write(std::cout, 10);
}

/** set matrix elements ito random values  in the range from  rMin to rMax*/
template <typename T> void Matrix<T>::setRandom(size_t seed, double rMin, double rMax) {
  MersenneTwister rng(seed, rMin, rMax);

  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      m_rawData[i][j] = static_cast<T>(rng.nextValue());
    }
  }
}

template <typename T>
void Matrix<T>::write(std::ostream &Fh, const int blockCnt) const
/**
Write out function for blocks of 10 Columns
@param Fh :: file stream for output
@param blockCnt :: number of columns per line (0 == full)
*/
{
  std::ios::fmtflags oldFlags = Fh.flags();
  Fh.setf(std::ios::floatfield, std::ios::scientific);
  const size_t blockNumber((blockCnt > 0) ? blockCnt : m_numColumns);
  size_t BCnt(0);
  do {
    const size_t ACnt = BCnt;
    BCnt += blockNumber;
    if (BCnt > m_numColumns) {
      BCnt = m_numColumns;
    }

    if (ACnt) {
      Fh << " ----- " << ACnt << " " << BCnt << " ------ \n";
    }
    for (size_t i = 0; i < m_numRows; i++) {
      for (size_t j = ACnt; j < BCnt; j++) {
        Fh << std::setw(10) << m_rawData[i][j] << "  ";
      }
      Fh << '\n';
    }
  } while (BCnt < m_numColumns);

  Fh.flags(oldFlags);
}

template <typename T>
std::string Matrix<T>::str() const
/**
Convert the matrix into a simple linear string expression
@return String value of output
*/
{
  std::ostringstream cx;
  for (size_t i = 0; i < m_numRows; i++) {
    for (size_t j = 0; j < m_numColumns; j++) {
      cx << std::setprecision(6) << m_rawData[i][j] << " ";
    }
  }
  return cx.str();
}

/**
 * Write an object to a stream. Format will be
 * Matrix(nrows,ncols)x_00,x_01...,x_10,x_11
 * @param os :: output stream
 * @param matrix :: Matrix to write out
 * @return The output stream (of)
 */
template <typename T> std::ostream &operator<<(std::ostream &os, const Matrix<T> &matrix) {
  dumpToStream(os, matrix, ',');
  return os;
}

/**
 * Write a Matrix to a stream. Format will be
 * Matrix(nrowsSEPncols)x_00SEPx_01...SEPx_10SEPx_11
 * @param os :: output stream
 * @param matrix :: Matrix to write out
 * @param delimiter :: A character to use as delimiter for the string
 */
template <typename T> void dumpToStream(std::ostream &os, const Kernel::Matrix<T> &matrix, const char delimiter) {
  size_t nrows(matrix.numRows()), ncols(matrix.numCols());
  os << "Matrix(" << nrows << delimiter << ncols << ")";
  for (size_t i = 0; i < nrows; ++i) {
    for (size_t j = 0; j < ncols; ++j) {
      os << matrix[i][j];
      if (i < nrows - 1 || j < ncols - 1)
        os << delimiter;
    }
  }
}

/**
 * Fill an object from a stream. Format should be
 * Matrix(nrows,ncols)x_00,x_01...,x_10,x_11
 * @param is :: A stream object
 * @param in :: An object to fill
 * @returns A reference to the stream
 */
template <typename T> std::istream &operator>>(std::istream &is, Kernel::Matrix<T> &in) {
  fillFromStream(is, in, ',');
  return is;
}

/**
 * Fill a Matrix from a stream using the given separator. Format should be
 * Matrix(nrowsSEPncols)x_00SEPx_01...SEPx_10SEPx_11
 * where SEP is replaced by the given separator
 * @param is :: A stream object
 * @param in :: An Matrix object to fill
 * @param delimiter :: A single character separator that delimits the entries
 */
template <typename T> void fillFromStream(std::istream &is, Kernel::Matrix<T> &in, const char delimiter) {
  // Stream should start with Matrix(
  char dump;
  std::string start(7, ' ');
  for (int i = 0; i < 7; ++i) {
    is >> dump;
    start[i] = dump;
    if (!is)
      throw std::invalid_argument("Unexpected character when reading Matrix from stream.");
  }
  if (start != "Matrix(")
    throw std::invalid_argument("Incorrect input format for Matrix stream.");
  // Now read a nrows,ncols and )
  size_t nrows(0), ncols(0);
  is >> nrows;
  if (!is)
    throw std::invalid_argument("Expected number of rows when reading Matrix "
                                "from stream, found something else.");
  is >> dump;
  is >> ncols;
  if (!is)
    throw std::invalid_argument("Expected number of columns when reading "
                                "Matrix from stream, found something else.");
  is >> dump;
  if (dump != ')')
    throw std::invalid_argument("Expected closing parenthesis after ncols when "
                                "reading Matrix from stream, found something "
                                "else.");

  // Resize the matrix
  in.setMem(nrows, ncols);

  // Use getline with the delimiter set to "," to read
  std::string value_str;
  size_t row(0), col(0);
  while (!is.eof() && std::getline(is, value_str, delimiter)) {
    try {
      auto value = boost::lexical_cast<T>(value_str);
      in.m_rawData[row][col] = value;
    } catch (boost::bad_lexical_cast &) {
      throw std::invalid_argument("Unexpected type found while reading Matrix from stream: \"" + value_str + "\"");
    }
    ++col;
    if (col == ncols) // New row
    {
      col = 0;
      ++row;
    }
  }
}

///\cond TEMPLATE

// Explicit instatiations. Avoid duplicate symbol definitions in
// client libraries. This must match the explicit declaration list
// in the header. MSVC/gcc differ in whether these symbols need to be
// marked as exported
#if defined(_MSC_VER)
#define KERNEL_MATRIX_SYMBOL_DLL MANTID_KERNEL_DLL
#else
#define KERNEL_MATRIX_SYMBOL_DLL
#endif
template class KERNEL_MATRIX_SYMBOL_DLL Matrix<double>;
template class KERNEL_MATRIX_SYMBOL_DLL Matrix<int>;
template class KERNEL_MATRIX_SYMBOL_DLL Matrix<float>;

template MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const DblMatrix &);
template MANTID_KERNEL_DLL void dumpToStream(std::ostream &, const DblMatrix &, const char);
template MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, DblMatrix &);
template MANTID_KERNEL_DLL void fillFromStream(std::istream &, DblMatrix &, const char);

template MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const Matrix<float> &);
template MANTID_KERNEL_DLL void dumpToStream(std::ostream &, const Matrix<float> &, const char);
template MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, Matrix<float> &);
template MANTID_KERNEL_DLL void fillFromStream(std::istream &, Matrix<float> &, const char);

template MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const IntMatrix &);
template MANTID_KERNEL_DLL void dumpToStream(std::ostream &, const IntMatrix &, const char);
template MANTID_KERNEL_DLL std::istream &operator>>(std::istream &, IntMatrix &);
template MANTID_KERNEL_DLL void fillFromStream(std::istream &, IntMatrix &, const char);
///\endcond TEMPLATE

} // namespace Mantid::Kernel
