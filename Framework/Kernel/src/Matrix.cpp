#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/MersenneTwister.h"

using Mantid::Kernel::TimeSeriesProperty;

namespace Mantid {

namespace Kernel {
//
#define fabs(x) std::fabs((x)*1.0)

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
  std::pair<T, int> operator()(const T &A) {
    return std::pair<T, int>(A, count++);
  }
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
template <typename T>
void indexSort(const std::vector<T> &pVec, std::vector<int> &Index) {
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
}

template <typename T> std::vector<T> Matrix<T>::getVector() const {
  std::vector<T> rez(nx * ny);
  size_t ic(0);
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      rez[ic] = V[i][j];
      ic++;
    }
  }
  return rez;
}
//
template <typename T>
Matrix<T>::Matrix(const size_t nrow, const size_t ncol, const bool makeIdentity)
    : nx(0), ny(0), V(nullptr)
/**
  Constructor with pre-set sizes. Matrix is zeroed
  @param nrow :: number of rows
  @param ncol :: number of columns
  @param makeIdentity :: flag for the constructor to return an identity matrix
*/
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(nrow, ncol);
  zeroMatrix();
  if (makeIdentity)
    identityMatrix();
}

template <typename T>
Matrix<T>::Matrix(const std::vector<T> &A, const std::vector<T> &B)
    : nx(0), ny(0), V(nullptr)
/**
  Constructor to take two vectors and multiply them to
  construct a matrix. (assuming that we have columns x row
  vector.
  @param A :: Column vector to multiply
  @param B :: Row vector to multiply
*/
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(A.size(), B.size());
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      V[i][j] = A[i] * B[j];
    }
  }
}
//
template <typename T>
Matrix<T>::Matrix(const std::vector<T> &data)
    : nx(0), ny(0), V(nullptr) {
  size_t numel = data.size();
  size_t nxt = static_cast<size_t>(sqrt(double(numel)));
  size_t test = nxt * nxt;
  if (test != numel) {
    throw(std::invalid_argument(
        "number of elements in input vector have to be square of some value"));
  }

  setMem(nxt, nxt);

  size_t ic(0);
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      V[i][j] = data[ic];
      ic++;
    }
  }
}

template <typename T>
Matrix<T>::Matrix(const std::vector<T> &data, const size_t nrow,
                  const size_t ncol)
    : nx(0), ny(0), V(nullptr) {
  size_t numel = data.size();
  size_t test = nrow * ncol;
  if (test != numel) {
    throw(std::invalid_argument("number of elements in input vector have is "
                                "incompatible with the number of rows and "
                                "columns"));
  }

  setMem(nrow, ncol);

  size_t ic(0);
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      V[i][j] = data[ic];
      ic++;
    }
  }
}

template <typename T>
Matrix<T>::Matrix(const Matrix<T> &A, const size_t nrow, const size_t ncol)
    : nx(A.nx - 1), ny(A.ny - 1), V(nullptr)
/**
  Constructor with for a missing row/column.
  @param A :: The input matrix
  @param nrow :: number of row to miss
  @param ncol :: number of column to miss
*/
{
  if (nrow > nx)
    throw Kernel::Exception::IndexError(nrow, A.nx,
                                        "Matrix::Constructor without col");
  if (ncol > ny)
    throw Kernel::Exception::IndexError(ncol, A.ny,
                                        "Matrix::Constructor without col");
  setMem(nx, ny);
  if (V) {
    size_t iR(0);
    for (size_t i = 0; i <= nx; i++) {
      if (i != nrow) {
        size_t jR(0);
        for (size_t j = 0; j <= ny; j++) {
          if (j != ncol) {

            V[iR][jR] = A.V[i][j];
            jR++;
          }
        }
        iR++;
      }
    }
  }
}

template <typename T>
Matrix<T>::Matrix(const Matrix<T> &A)
    : nx(0), ny(0), V(nullptr)
/**
  Simple copy constructor
  @param A :: Object to copy
*/
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(A.nx, A.ny);
  if (nx * ny) {
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = 0; j < ny; j++) {
        V[i][j] = A.V[i][j];
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
    setMem(A.nx, A.ny);
    if (nx * ny) {
      for (size_t i = 0; i < nx; i++) {
        for (size_t j = 0; j < ny; j++) {
          V[i][j] = A.V[i][j];
        }
      }
    }
  }
  return *this;
}

template <typename T>
Matrix<T>::Matrix(Matrix<T> &&other) noexcept : nx(other.nx),
                                                ny(other.ny),
                                                V(other.V) {
  other.nx = 0;
  other.ny = 0;
  other.V = nullptr;
}

template <typename T>
Matrix<T> &Matrix<T>::operator=(Matrix<T> &&other) noexcept {
  nx = other.nx;
  ny = other.ny;
  V = other.V;

  other.nx = 0;
  other.ny = 0;
  other.V = nullptr;

  return *this;
}

template <typename T>
Matrix<T>::~Matrix()
/**
  Delete operator :: removes memory for
  matrix
*/
{
  deleteMem();
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
  const size_t Xpt((nx > A.nx) ? A.nx : nx);
  const size_t Ypt((ny > A.ny) ? A.ny : ny);
  for (size_t i = 0; i < Xpt; i++) {
    for (size_t j = 0; j < Ypt; j++) {
      V[i][j] += A.V[i][j];
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
  const size_t Xpt((nx > A.nx) ? A.nx : nx);
  const size_t Ypt((ny > A.ny) ? A.ny : ny);
  for (size_t i = 0; i < Xpt; i++) {
    for (size_t j = 0; j < Ypt; j++) {
      V[i][j] -= A.V[i][j];
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
  if (ny != A.nx)
    throw Kernel::Exception::MisMatch<size_t>(ny, A.nx,
                                              "Matrix::operator*(Matrix)");
  Matrix<T> X(nx, A.ny);
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < A.ny; j++) {
      for (size_t kk = 0; kk < ny; kk++) {
        X.V[i][j] += V[i][kk] * A.V[kk][j];
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
  if (ny > Vec.size())
    throw Kernel::Exception::MisMatch<size_t>(ny, Vec.size(),
                                              "Matrix::operator*(Vec)");

  std::vector<T> Out(nx);
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      Out[i] += V[i][j] * Vec[j];
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
template <typename T>
void Matrix<T>::multiplyPoint(const std::vector<T> &in,
                              std::vector<T> &out) const {
  out.resize(nx);
  std::fill(std::begin(out), std::end(out), static_cast<T>(0.0));
  if (ny > in.size())
    throw Kernel::Exception::MisMatch<size_t>(ny, in.size(),
                                              "Matrix::multiplyPoint(in,out)");
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      out[i] += V[i][j] * in[j];
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
  if (ny != 3 || nx > 3)
    throw Kernel::Exception::MisMatch<size_t>(ny, 3, "Matrix::operator*(V3D)");

  V3D v;
  for (size_t i = 0; i < nx; ++i) {
    v[i] = V[i][0] * Vx.X() + V[i][1] * Vx.Y() + V[i][2] * Vx.Z();
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
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      X.V[i][j] *= Value;
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
  if (ny != A.nx)
    throw Kernel::Exception::MisMatch<size_t>(ny, A.nx, "Matrix*=(Matrix<T>)");
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
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      V[i][j] *= Value;
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
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      V[i][j] /= Value;
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
    if (A.nx != nx || A.ny != ny)
      return false;

    double maxS(0.0);
    double maxDiff(0.0); // max di
    for (size_t i = 0; i < nx; i++)
      for (size_t j = 0; j < ny; j++) {
        const T diff = (V[i][j] - A.V[i][j]);
        if (fabs(diff) > maxDiff)
          maxDiff = fabs(diff);
        if (fabs(V[i][j]) > maxS)
          maxS = fabs(V[i][j]);
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

  if (A.nx != nx || A.ny != ny)
    return false;

  for (size_t i = 0; i < nx; i++)
    for (size_t j = 0; j < ny; j++) {
      if (V[i][j] >= A.V[i][j])
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

  if (A.nx != nx || A.ny != ny)
    return false;

  for (size_t i = 0; i < nx; i++)
    for (size_t j = 0; j < ny; j++) {
      if (V[i][j] < A.V[i][j])
        return false;
    }
  return true;
}

//---------------------------------------------------------------------------------------
template <typename T>
void Matrix<T>::deleteMem()
/**
  Deletes the memory held in matrix
*/
{
  if (V) {
    delete[] * V;
    delete[] V;
    V = nullptr;
  }
  nx = 0;
  ny = 0;
}

/**
  Sets the memory held in matrix
  @param a :: number of rows
  @param b :: number of columns
*/
template <typename T> void Matrix<T>::setMem(const size_t a, const size_t b) {
  if (a == nx && b == ny && V != nullptr)
    return;

  deleteMem();
  if (a <= 0 || b <= 0)
    return;

  nx = a;
  ny = b;
  if (nx * ny) {
    auto tmpX = new T[nx * ny];
    V = new T *[nx];
    for (size_t i = 0; i < nx; i++) {
      V[i] = tmpX + (i * ny);
    }
  }
}

/**
  Swap rows I and J
  @param RowI :: row I to swap
  @param RowJ :: row J to swap
*/
template <typename T>
void Matrix<T>::swapRows(const size_t RowI, const size_t RowJ) {
  if (nx * ny && RowI < nx && RowJ < nx && RowI != RowJ) {
    for (size_t k = 0; k < ny; k++) {
      T tmp = V[RowI][k];
      V[RowI][k] = V[RowJ][k];
      V[RowJ][k] = tmp;
    }
  }
}

/**
  Swap columns I and J
  @param colI :: col I to swap
  @param colJ :: col J to swap
*/
template <typename T>
void Matrix<T>::swapCols(const size_t colI, const size_t colJ) {
  if (nx * ny && colI < ny && colJ < ny && colI != colJ) {
    for (size_t k = 0; k < nx; k++) {
      T tmp = V[k][colI];
      V[k][colI] = V[k][colJ];
      V[k][colJ] = tmp;
    }
  }
}

template <typename T>
void Matrix<T>::zeroMatrix()
/**
  Zeros all elements of the matrix
*/
{
  if (nx * ny) {
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = 0; j < ny; j++) {
        V[i][j] = static_cast<T>(0);
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
  if (nx * ny) {
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = 0; j < ny; j++) {
        V[i][j] = static_cast<T>(j == i);
      }
    }
  }
}
template <typename T>
void Matrix<T>::setColumn(const size_t nCol, const std::vector<T> &newCol) {
  if (nCol >= this->ny) {
    throw(std::invalid_argument("nCol requested> nCol availible"));
  }
  size_t nxM = newCol.size();
  if (nx < nxM)
    nxM = nx;
  for (size_t i = 0; i < nxM; i++) {
    V[i][nCol] = newCol[i];
  }
}
template <typename T>
void Matrix<T>::setRow(const size_t nRow, const std::vector<T> &newRow) {
  if (nRow >= this->nx) {
    throw(std::invalid_argument("nRow requested> nRow availible"));
  }
  size_t nyM = newRow.size();
  if (ny < nyM)
    nyM = ny;
  for (size_t j = 0; j < nyM; j++) {
    V[nRow][j] = newRow[j];
  }
}

template <typename T>
void Matrix<T>::rotate(const double tau, const double s, const int i,
                       const int j, const int k, const int m)
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
  const T gg = V[i][j];
  const T hh = V[k][m];
  V[i][j] = static_cast<T>(gg - s * (hh + gg * tau));
  V[k][m] = static_cast<T>(hh + s * (gg - hh * tau));
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
  if (Dvec.size() != nx) {
    std::ostringstream cx;
    cx << "Matrix::preMultiplyByDiagonal Size: " << Dvec.size() << " " << nx
       << " " << ny;
    throw std::runtime_error(cx.str());
  }
  Matrix<T> X(Dvec.size(), ny);
  for (size_t i = 0; i < Dvec.size(); i++) {
    for (size_t j = 0; j < ny; j++) {
      X.V[i][j] = Dvec[i] * V[i][j];
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
  if (Dvec.size() != ny) {
    std::ostringstream cx;
    cx << "Error Matrix::bDiaognal size:: " << Dvec.size() << " " << nx << " "
       << ny;
    throw std::runtime_error(cx.str());
  }

  Matrix<T> X(nx, Dvec.size());
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < Dvec.size(); j++) {
      X.V[i][j] = Dvec[j] * V[i][j];
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
  if (!nx * ny)
    return *this;

  if (nx == ny) // inplace transpose
  {
    Matrix<T> MT(*this);
    MT.Transpose();
    return MT;
  }

  // irregular matrix
  Matrix<T> MT(ny, nx);
  for (size_t i = 0; i < nx; i++)
    for (size_t j = 0; j < ny; j++)
      MT.V[j][i] = V[i][j];

  return MT;
}

template <typename T>
Matrix<T> &Matrix<T>::Transpose()
/**
  Transpose the matrix :
  Has a inplace transpose for a square matrix case.
  @return this^T
*/
{
  if (!nx * ny)
    return *this;
  if (nx == ny) // inplace transpose
  {
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = i + 1; j < ny; j++) {
        T tmp = V[i][j];
        V[i][j] = V[j][i];
        V[j][i] = tmp;
      }
    }
    return *this;
  }
  // irregular matrix
  // get some memory
  auto tmpX = new T[ny * nx];
  auto Vt = new T *[ny];
  for (size_t i = 0; i < ny; i++) {
    Vt[i] = tmpX + (i * nx);
  }
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      Vt[j][i] = V[i][j];
    }
  }
  // remove old memory
  const size_t tx = nx;
  const size_t ty = ny;
  deleteMem(); // resets nx,ny
  // replace memory
  V = Vt;
  nx = ty;
  ny = tx;

  return *this;
}

template <>
void Matrix<int>::GaussJordan(Kernel::Matrix<int> &)
/**
  Not valid for Integer
  @throw std::invalid_argument
*/
{
  throw std::invalid_argument(
      "Gauss-Jordan inversion not valid for integer matrix");
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
  if (nx != ny || B.nx != nx) {
    throw std::invalid_argument("Matrix not square, or sizes do not match");
  }

  // pivoted rows
  std::vector<int> pivoted(nx);
  fill(pivoted.begin(), pivoted.end(), 0);

  std::vector<int> indxcol(nx); // Column index
  std::vector<int> indxrow(nx); // row index

  size_t irow(0), icol(0);
  for (size_t i = 0; i < nx; i++) {
    // Get Biggest non-pivoted item
    double bigItem = 0.0; // get point to pivot over
    for (size_t j = 0; j < nx; j++) {
      if (pivoted[j] != 1) // check only non-pivots
      {
        for (size_t k = 0; k < nx; k++) {
          if (!pivoted[k]) {
            if (fabs(V[j][k]) >= bigItem) {
              bigItem = fabs(V[j][k]);
              irow = j;
              icol = k;
            }
          }
        }
      } else if (pivoted[j] > 1) {
        throw std::runtime_error("Error doing G-J elem on a singular matrix");
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

    if (V[icol][icol] == 0.0) {
      throw std::runtime_error("Error doing G-J elem on a singular matrix");
    }
    const T pivDiv = T(1.0) / V[icol][icol];
    V[icol][icol] = 1;
    for (size_t l = 0; l < nx; l++) {
      V[icol][l] *= pivDiv;
    }
    for (size_t l = 0; l < B.ny; l++) {
      B.V[icol][l] *= pivDiv;
    }

    for (size_t ll = 0; ll < nx; ll++) {
      if (ll != icol) {
        const T div_num = V[ll][icol];
        V[ll][icol] = 0.0;
        for (size_t l = 0; l < nx; l++) {
          V[ll][l] -= V[icol][l] * div_num;
        }
        for (size_t l = 0; l < B.ny; l++) {
          B.V[ll][l] -= B.V[icol][l] * div_num;
        }
      }
    }
  }

  // Un-roll interchanges
  if (nx > 0) {
    for (int l = static_cast<int>(nx) - 1; l >= 0; l--) {
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
  if (nx != ny && nx < 1)
    return 0;

  if (nx == 1) {
    T det = V[0][0];
    if (V[0][0] != static_cast<T>(0.))
      V[0][0] = static_cast<T>(1.) / V[0][0];
    return det;
  }
  auto indx = new int[nx]; // Set in lubcmp

  auto col = new double[nx];
  int d;
  Matrix<T> Lcomp(*this);
  Lcomp.lubcmp(indx, d);

  double det = static_cast<double>(d);
  for (size_t j = 0; j < nx; j++)
    det *= Lcomp.V[j][j];

  for (size_t j = 0; j < nx; j++) {
    for (size_t i = 0; i < nx; i++)
      col[i] = 0.0;
    col[j] = 1.0;
    Lcomp.lubksb(indx, col);
    for (size_t i = 0; i < nx; i++)
      V[i][j] = static_cast<T>(col[i]);
  }
  delete[] indx;
  delete[] col;
  return static_cast<T>(det);
}

template <typename T>
T Matrix<T>::determinant() const
/**
  Calculate the derminant of the matrix
  @return Determinant of matrix.
*/
{
  if (nx != ny)
    throw Kernel::Exception::MisMatch<size_t>(
        nx, ny, "Determinant error :: Matrix is not NxN");

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
  if (nx != ny || nx < 1)
    throw std::runtime_error("Matrix::factor Matrix is not NxN");

  double deter = 1.0;
  for (int i = 0; i < static_cast<int>(nx) - 1; i++) // loop over each row
  {
    int jmax = i;
    double Pmax = fabs(V[i][i]);
    for (int j = i + 1; j < static_cast<int>(nx); j++) // find max in Row i
    {
      if (fabs(V[i][j]) > Pmax) {
        Pmax = fabs(V[i][j]);
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
    Pmax = V[i][i];
    deter *= Pmax;
    for (int k = i + 1; k < static_cast<int>(nx); k++) // row index
    {
      const double scale = V[k][i] / Pmax;
      V[k][i] = static_cast<T>(0);
      for (int q = i + 1; q < static_cast<int>(nx); q++) // column index
        V[k][q] -= static_cast<T>(scale * V[i][q]);
    }
  }
  deter *= V[nx - 1][nx - 1];
  return static_cast<T>(deter);
}

template <typename T>
void Matrix<T>::normVert()
/**
  Normalise EigenVectors
  Assumes that they have already been calculated
*/
{
  for (size_t i = 0; i < nx; i++) {
    T sum = 0;
    for (size_t j = 0; j < ny; j++) {
      sum += V[i][j] * V[i][j];
    }
    sum = static_cast<T>(std::sqrt(static_cast<double>(sum)));
    for (size_t j = 0; j < ny; j++) {
      V[i][j] /= sum;
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
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      sum += V[i][j] * V[i][j];
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
  @param rowperm :: row permutations [nx values]
*/
{
  double sum, dum, big, temp;

  if (nx != ny || nx < 2) {
    std::cerr << "Error with lubcmp\n";
    return;
  }
  auto vv = new double[nx];
  interchange = 1;
  for (int i = 0; i < static_cast<int>(nx); i++) {
    big = 0.0;
    for (int j = 0; j < static_cast<int>(nx); j++)
      if ((temp = fabs(V[i][j])) > big)
        big = temp;

    if (big == 0.0) {
      delete[] vv;
      for (int j = 0; j < static_cast<int>(nx); j++) {
        rowperm[j] = j;
      }
      return;
    }
    vv[i] = 1.0 / big;
  }

  for (int j = 0; j < static_cast<int>(nx); j++) {
    for (int i = 0; i < j; i++) {
      sum = V[i][j];
      for (int k = 0; k < i; k++)
        sum -= V[i][k] * V[k][j];
      V[i][j] = static_cast<T>(sum);
    }
    big = 0.0;
    int imax = j;
    for (int i = j; i < static_cast<int>(nx); i++) {
      sum = V[i][j];
      for (int k = 0; k < j; k++)
        sum -= V[i][k] * V[k][j];
      V[i][j] = static_cast<T>(sum);
      if ((dum = vv[i] * fabs(sum)) >= big) {
        big = dum;
        imax = i;
      }
    }

    if (j != imax) {
      for (int k = 0; k < static_cast<int>(nx); k++) { // Interchange rows
        dum = V[imax][k];
        V[imax][k] = V[j][k];
        V[j][k] = static_cast<T>(dum);
      }
      interchange *= -1;
      vv[imax] = static_cast<T>(vv[j]);
    }
    rowperm[j] = imax;

    if (V[j][j] == 0.0)
      V[j][j] = static_cast<T>(1e-14);
    if (j != static_cast<int>(nx) - 1) {
      dum = 1.0 / (V[j][j]);
      for (int i = j + 1; i < static_cast<int>(nx); i++)
        V[i][j] *= static_cast<T>(dum);
    }
  }
  delete[] vv;
}

template <typename T>
void Matrix<T>::lubksb(const int *rowperm, double *b)
/**
  Impliments a separation of the Matrix
  into a triangluar matrix
*/
{
  int ii = -1;

  for (int i = 0; i < static_cast<int>(nx); i++) {
    int ip = rowperm[i];
    double sum = b[ip];
    b[ip] = b[i];
    if (ii != -1)
      for (int j = ii; j < i; j++)
        sum -= V[i][j] * b[j];
    else if (sum != 0.)
      ii = i;
    b[i] = sum;
  }

  for (int i = static_cast<int>(nx) - 1; i >= 0; i--) {
    double sum = static_cast<T>(b[i]);
    for (int j = i + 1; j < static_cast<int>(nx); j++)
      sum -= V[i][j] * b[j];
    b[i] = sum / V[i][i];
  }
}

template <typename T>
void Matrix<T>::averSymmetric()
/**
  Simple function to create an average symmetric matrix
  out of the Matrix
*/
{
  const size_t minSize = (nx > ny) ? ny : nx;
  for (size_t i = 0; i < minSize; i++) {
    for (size_t j = i + 1; j < minSize; j++) {
      V[i][j] = (V[i][j] + V[j][i]) / 2;
      V[j][i] = V[i][j];
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
  const size_t Msize = (ny > nx) ? nx : ny;
  std::vector<T> Diag(Msize);
  for (size_t i = 0; i < Msize; i++) {
    Diag[i] = V[i][i];
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
  const size_t Msize = (ny > nx) ? nx : ny;
  T Trx = 0;
  for (size_t i = 0; i < Msize; i++) {
    Trx += V[i][i];
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
  if (ny != nx || nx != DiagMatrix.nx || nx != DiagMatrix.ny) {
    std::cerr << "Matrix not Eigen Form\n";
    throw(std::invalid_argument(" Matrix is not in an eigenvalue format"));
  }
  std::vector<int> index;
  std::vector<T> X = DiagMatrix.Diagonal();
  indexSort(X, index);
  Matrix<T> EigenVec(*this);
  for (size_t Icol = 0; Icol < nx; Icol++) {
    for (size_t j = 0; j < nx; j++) {
      V[j][Icol] = EigenVec[j][index[Icol]];
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
  if (nx != ny || nx < 1) {
    std::cerr << "Matrix not square\n";
    return 0;
  }
  for (size_t i = 0; i < nx; i++)
    for (size_t j = i + 1; j < nx; j++)
      if (fabs(V[i][j] - V[j][i]) > 1e-6) {
        std::cerr << "Matrix not symmetric\n";
        std::cerr << (*this);
        return 0;
      }

  Matrix<T> A(*this);
  // Make V an identity matrix
  EigenVec.setMem(nx, nx);
  EigenVec.identityMatrix();
  DiagMatrix.setMem(nx, nx);
  DiagMatrix.zeroMatrix();

  std::vector<double> Diag(nx);
  std::vector<double> B(nx);
  std::vector<double> ZeroComp(nx);
  // set b and d to the diagonal elements o A
  for (size_t i = 0; i < nx; i++) {
    Diag[i] = B[i] = A.V[i][i];
    ZeroComp[i] = 0;
  }

  int iteration = 0;
  for (int i = 0; i < 100; i++) // max 50 iterations
  {
    double sm = 0.0; // sum of off-diagonal terms
    for (size_t ip = 0; ip < nx - 1; ip++)
      for (size_t iq = ip + 1; iq < nx; iq++)
        sm += fabs(A.V[ip][iq]);

    if (sm == 0.0) // Nothing to do return...
    {
      // Make OUTPUT -- D + A
      // sort Output::
      for (size_t ix = 0; ix < nx; ix++)
        DiagMatrix.V[ix][ix] = static_cast<T>(Diag[ix]);
      return 1;
    }

    // Threshold large for first 5 sweeps
    double tresh = (i < 6) ? 0.2 * sm / static_cast<int>(nx * nx) : 0.0;

    for (int ip = 0; ip < static_cast<int>(nx) - 1; ip++) {
      for (int iq = ip + 1; iq < static_cast<int>(nx); iq++) {
        double g = 100.0 * fabs(A.V[ip][iq]);
        // After 4 sweeps skip if off diagonal small
        if (i > 6 &&
            static_cast<float>(fabs(Diag[ip] + g)) ==
                static_cast<float>(fabs(Diag[ip])) &&
            static_cast<float>(fabs(Diag[iq] + g)) ==
                static_cast<float>(fabs(Diag[iq])))
          A.V[ip][iq] = 0;

        else if (fabs(A.V[ip][iq]) > tresh) {
          double tanAngle, cosAngle, sinAngle;
          double h = Diag[iq] - Diag[ip];
          if (static_cast<float>((fabs(h) + g)) == static_cast<float>(fabs(h)))
            tanAngle = A.V[ip][iq] / h; // tanAngle=1/(2theta)
          else {
            double theta = 0.5 * h / A.V[ip][iq];
            tanAngle = 1.0 / (fabs(theta) + sqrt(1.0 + theta * theta));
            if (theta < 0.0)
              tanAngle = -tanAngle;
          }
          cosAngle = 1.0 / sqrt(1 + tanAngle * tanAngle);
          sinAngle = tanAngle * cosAngle;
          double tau = sinAngle / (1.0 + cosAngle);
          h = tanAngle * A.V[ip][iq];
          ZeroComp[ip] -= h;
          ZeroComp[iq] += h;
          Diag[ip] -= h;
          Diag[iq] += h;
          A.V[ip][iq] = 0;
          // Rotations 0<j<p
          for (int j = 0; j < ip; j++)
            A.rotate(tau, sinAngle, j, ip, j, iq);
          for (int j = ip + 1; j < iq; j++)
            A.rotate(tau, sinAngle, ip, j, j, iq);
          for (int j = iq + 1; j < static_cast<int>(nx); j++)
            A.rotate(tau, sinAngle, ip, j, iq, j);
          for (int j = 0; j < static_cast<int>(nx); j++)
            EigenVec.rotate(tau, sinAngle, j, ip, j, iq);
          iteration++;
        }
      }
    }
    for (size_t j = 0; j < nx; j++) {
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
  if (this->nx != this->ny)
    throw(std::invalid_argument("matrix is not square"));
  //  std::cout << "Matrix determinant-1 is " << (this->determinant()-1) <<
  //  '\n';
  if (fabs(this->determinant() - 1) > 1e-5) {
    return false;
  } else {
    Matrix<T> prod(nx, ny), ident(nx, ny, true);
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
  if (this->nx != this->ny)
    throw(std::invalid_argument("matrix is not square"));
  if (fabs(fabs(this->determinant()) - 1.) > 1e-5) {
    return false;
  } else {
    Matrix<T> prod(nx, ny), ident(nx, ny, true);
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
  if (this->nx != this->ny)
    throw(std::invalid_argument("matrix is not square"));
  if (fabs(this->determinant()) < 1e-10)
    throw(std::invalid_argument("Determinant is too small"));
  // step 1: orthogonalize the matrix
  for (size_t i = 0; i < this->ny; ++i) {
    double spself = 0.;
    for (size_t j = 0; j < this->nx; ++j)
      spself += (V[j][i] * V[j][i]);
    for (size_t k = i + 1; k < this->ny; ++k) {
      double spother = 0;
      for (size_t j = 0; j < this->nx; ++j)
        spother += (V[j][i] * V[j][k]);
      for (size_t j = 0; j < this->nx; ++j)
        V[j][k] -= static_cast<T>(V[j][i] * spother / spself);
    }
  }
  // step 2: get scales and rescsale the matrix
  std::vector<T> scale(this->nx);
  T currentScale;
  for (size_t i = 0; i < this->ny; ++i) {
    currentScale = T(0.);
    for (size_t j = 0; j < this->nx; ++j)
      currentScale += (V[j][i] * V[j][i]);
    currentScale = static_cast<T>(sqrt(static_cast<double>(currentScale)));
    if (currentScale < 1e-10)
      throw(std::invalid_argument("Scale is too small"));
    scale[i] = currentScale;
  }
  Matrix<T> scalingMatrix(nx, ny), change(nx, ny, true);
  for (size_t i = 0; i < this->ny; ++i)
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
template <typename T>
void Matrix<T>::setRandom(size_t seed, double rMin, double rMax) {
  MersenneTwister rng(seed, rMin, rMax);

  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      V[i][j] = static_cast<T>(rng.nextValue());
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
  const size_t blockNumber((blockCnt > 0) ? blockCnt : ny);
  size_t BCnt(0);
  do {
    const size_t ACnt = BCnt;
    BCnt += blockNumber;
    if (BCnt > ny) {
      BCnt = ny;
    }

    if (ACnt) {
      Fh << " ----- " << ACnt << " " << BCnt << " ------ \n";
    }
    for (size_t i = 0; i < nx; i++) {
      for (size_t j = ACnt; j < BCnt; j++) {
        Fh << std::setw(10) << V[i][j] << "  ";
      }
      Fh << '\n';
    }
  } while (BCnt < ny);

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
  for (size_t i = 0; i < nx; i++) {
    for (size_t j = 0; j < ny; j++) {
      cx << std::setprecision(6) << V[i][j] << " ";
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
template <typename T>
std::ostream &operator<<(std::ostream &os, const Matrix<T> &matrix) {
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
template <typename T>
void dumpToStream(std::ostream &os, const Kernel::Matrix<T> &matrix,
                  const char delimiter) {
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
template <typename T>
std::istream &operator>>(std::istream &is, Kernel::Matrix<T> &in) {
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
template <typename T>
void fillFromStream(std::istream &is, Kernel::Matrix<T> &in,
                    const char delimiter) {
  // Stream should start with Matrix(
  char dump;
  std::string start(7, ' ');
  for (int i = 0; i < 7; ++i) {
    is >> dump;
    start[i] = dump;
    if (!is)
      throw std::invalid_argument(
          "Unexpected character when reading Matrix from stream.");
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
      T value = boost::lexical_cast<T>(value_str);
      in.V[row][col] = value;
    } catch (boost::bad_lexical_cast &) {
      throw std::invalid_argument(
          "Unexpected type found while reading Matrix from stream: \"" +
          value_str + "\"");
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

// Symbol definitions for common types
template class MANTID_KERNEL_DLL Matrix<double>;
// The explicit template instantiation for int does not have an export macro
// since this produces a warning on "gcc: warning: type attributes ignored after
// type is already define" The reason for this is the use of Matrix<int>
// in a template specialization above, causing an implicit sepcialization.
// This, most likely, obtains a visibility setting from the general template
// definition.
template class Matrix<int>;
template class MANTID_KERNEL_DLL Matrix<float>;

template MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                                    const DblMatrix &);
template MANTID_KERNEL_DLL void dumpToStream(std::ostream &, const DblMatrix &,
                                             const char);
template MANTID_KERNEL_DLL std::istream &operator>>(std::istream &,
                                                    DblMatrix &);
template MANTID_KERNEL_DLL void fillFromStream(std::istream &, DblMatrix &,
                                               const char);

template MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                                    const Matrix<float> &);
template MANTID_KERNEL_DLL void dumpToStream(std::ostream &,
                                             const Matrix<float> &, const char);
template MANTID_KERNEL_DLL std::istream &operator>>(std::istream &,
                                                    Matrix<float> &);
template MANTID_KERNEL_DLL void fillFromStream(std::istream &, Matrix<float> &,
                                               const char);

template MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                                    const IntMatrix &);
template MANTID_KERNEL_DLL void dumpToStream(std::ostream &, const IntMatrix &,
                                             const char);
template MANTID_KERNEL_DLL std::istream &operator>>(std::istream &,
                                                    IntMatrix &);
template MANTID_KERNEL_DLL void fillFromStream(std::istream &, IntMatrix &,
                                               const char);
///\endcond TEMPLATE

} // namespace Kernel
} // namespace
