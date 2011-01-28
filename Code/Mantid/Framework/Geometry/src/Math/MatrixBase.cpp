#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <complex>
#include <vector>
#include <boost/array.hpp>
#include <boost/multi_array.hpp>

#include "MantidKernel/Exception.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/MatrixBase.h"
#include "MantidGeometry/Math/PolyFunction.h"
#include "MantidGeometry/Math/PolyVar.h"

namespace Mantid
{

template<typename T>
std::ostream&
Geometry::operator<<(std::ostream& OX,const Geometry::MatrixBase<T>& A)
  /**
    External outputs point to a stream
    @param A :: MatrixBase to write out
    @param OX :: output stream
    @return The output stream (of)
  */
{
  OX<<std::endl;
  A.write(OX,5);
  return OX;
}

template<int index>
std::ostream&
Geometry::operator<<(std::ostream& OX,
     const Geometry::MatrixBase<mathLevel::PolyVar<index> >& A)
  /**
    External outputs point to a stream
    @param A :: MatrixBase to write out
    @param OX :: output stream
    @return The output stream (OX)
  */
{
  OX<<std::endl;
  A.writeGrid(OX);
  return OX;
}

namespace Geometry
{

template<typename T>
MatrixBase<T>::MatrixBase(const int nrow,const int ncol)
  : nx(0),ny(0),V(0)
  /**
    Constructor with pre-set sizes. MatrixBase is zeroed
    @param nrow :: number of rows
    @param ncol :: number of columns
  */
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(nrow,ncol);
  zeroMatrix();
}

template<typename T>
MatrixBase<T>::MatrixBase(const std::vector<T>& A,const std::vector<T>& B)
  : nx(0),ny(0),V(0)
  /**
    Constructor to take two vectors and multiply them to
    construct a matrix. (assuming that we have columns x row
    vector.)
    @param A :: Column vector to multiply
    @param B :: Row vector to multiply
  */
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(A.size(),B.size());
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      V[i][j]=A[i]*B[j];
}

template<typename T>
MatrixBase<T>::MatrixBase(const MatrixBase<T>& A,const int nrow,const int ncol)
  : nx(A.nx-1),ny(A.ny-1),V(0)
  /**
    Constructor with for a missing row/column.
    @param A :: The input matrix
    @param nrow :: number of row to miss
    @param ncol :: number of column to miss
  */
{
  // Note:: nx,ny zeroed so setMem always works
  if (nrow>nx || nrow<0)
    throw Kernel::Exception::IndexError(nrow,A.nx,
			     "MatrixBase::Constructor without col");
  if (ncol>ny || ncol<0)
    throw Kernel::Exception::IndexError(ncol,A.ny,
			     "MatrixBase::Constructor without col");
  std::cerr<<"Setting MBASE Meme"<<std::endl;
  setMem(nx,ny);
  int iR(0);
  for(int i=0;i<=nx;i++)
    {
      if (i!=nrow)
        {
	  int jR(0);
	  for(int j=0;j<=ny;j++)
	    if (j!=ncol)
	      {
		V[iR][jR]=A.V[i][j];
		jR++;
	      }
	  iR++;
	}
    }
  return;
}

template<typename T>
MatrixBase<T>::MatrixBase(const MatrixBase<T>& A)
  : nx(0),ny(0),V(0)
  /**
    Simple copy constructor
    @param A :: Object to copy
  */
{
  // Note:: nx,ny zeroed so setMem always works
  setMem(A.nx,A.ny);
  if (nx*ny)
    {
      for(int i=0;i<nx;i++)
	for(int j=0;j<ny;j++)
	  V[i][j]=A.V[i][j];
    }
}


template<typename T>
MatrixBase<T>&
MatrixBase<T>::operator=(const MatrixBase<T>& A)
  /**
    Simple assignment operator
    @param A :: Object to copy
    @return the copied object
  */
{
  if (&A!=this)
    {
      setMem(A.nx,A.ny);
      if (nx*ny)
	for(int i=0;i<nx;i++)
	  for(int j=0;j<ny;j++)
	    V[i][j]=A.V[i][j];
    }
  return *this;
}

template<typename T>
MatrixBase<T>::~MatrixBase()
  /**
    Delete operator :: removes memory for
    matrix
  */
{
  deleteMem();
}


template<typename T>
MatrixBase<T>&
MatrixBase<T>::operator+=(const MatrixBase<T>& A)
   /**
     MatrixBase addition THIS + A
     If the size is different then 0 is added where appropiate
     MatrixBase A is not expanded.
     @param A :: MatrixBase to add
     @return MatrixBase(this + A)
   */
{
  const int Xpt((nx>A.nx) ? A.nx : nx);
  const int Ypt((ny>A.ny) ? A.ny : ny);
  for(int i=0;i<Xpt;i++)
    for(int j=0;j<Ypt;j++)
      V[i][j]+=A.V[i][j];

  return *this;
}

template<typename T>
MatrixBase<T>&
MatrixBase<T>::operator-=(const MatrixBase<T>& A)
   /**
     MatrixBase subtractoin THIS - A
     If the size is different then 0 is added where appropiate
     MatrixBase A is not expanded.
     @param A :: MatrixBase to add
     @return Ma
   */
{
  const int Xpt((nx>A.nx) ? A.nx : nx);
  const int Ypt((ny>A.ny) ? A.ny : ny);
  for(int i=0;i<Xpt;i++)
    for(int j=0;j<Ypt;j++)
      V[i][j]-=A.V[i][j];

  return *this;
}

template<typename T>
MatrixBase<T>
MatrixBase<T>::operator+(const MatrixBase<T>& A)
   /**
     MatrixBase addition THIS + A
     If the size is different then 0 is added where appropiate
     MatrixBase A is not expanded.
     @param A :: MatrixBase to add
     @return MatrixBase(this + A)
   */
{
  MatrixBase<T> X(*this);
  return X+=A;
}

template<typename T>
MatrixBase<T>
MatrixBase<T>::operator-(const MatrixBase<T>& A)
   /**
     MatrixBase subtraction THIS - A
     If the size is different then 0 is subtracted where
     appropiate. This matrix determines the size
     @param A :: MatrixBase to add
     @return MatrixBase(this + A)
   */
{
  MatrixBase<T> X(*this);
  return X-=A;
}

template<typename T>
MatrixBase<T>
MatrixBase<T>::operator*(const MatrixBase<T>& A) const
  /**
    MatrixBase multiplication THIS * A
    @param A :: MatrixBase to multiply by  (this->row must == A->columns)
    @throw MisMatch<int> if there is a size mismatch.
    @return MatrixBase(This * A)
 */
{
  if (ny!=A.nx)
    throw Kernel::Exception::MisMatch<int>(ny,A.nx,"MatrixBase::operator*(MatrixBase)");
  MatrixBase<T> X(nx,A.ny);
  for(int i=0;i<nx;i++)
    for(int j=0;j<A.ny;j++)
      for(int kk=0;kk<ny;kk++)
	X.V[i][j]+=V[i][kk]*A.V[kk][j];
  return X;
}

template<typename T>
std::vector<T>
MatrixBase<T>::operator*(const std::vector<T>& Vec) const
  /**
    MatrixBase multiplication THIS * Vec to produce a vec
    @param Vec :: size of vector > this->nrows
    @throw MisMatch<int> if there is a size mismatch.
    @return MatrixBase(This * Vec)
  */
{
  std::vector<T> Out;
  if (ny>static_cast<int>(Vec.size()))
    throw Kernel::Exception::MisMatch<int>(ny,Vec.size(),"MatrixBase::operator*(Vec)");

  Out.resize(nx);
  for(int i=0;i<nx;i++)
    {
      Out[i]=0;
      for(int j=0;j<ny;j++)
	Out[i]+=V[i][j]*Vec[j];
    }
  return Out;
}

template<typename T>
MatrixBase<T>
MatrixBase<T>::operator*(const T& Value) const
   /**
     MatrixBase multiplication THIS * Value
     @param Value :: Scalar to multiply by
     @return V * (this)
   */
{
  MatrixBase<T> X(*this);
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      X.V[i][j]*=Value;
  return X;
}

template<typename T>
MatrixBase<T>&
MatrixBase<T>::operator*=(const MatrixBase<T>& A)
   /**
     MatrixBase multiplication THIS *= A
     Note that we call operator* to avoid the problem
     of changing matrix size.
    @param A :: MatrixBase to multiply by  (this->row must == A->columns)
    @return This *= A
   */
{
  if (ny!=A.nx)
    throw Kernel::Exception::MisMatch<int>(ny,A.nx,"MatrixBase*=(MatrixBase<T>)");
  // This construct to avoid the problem of changing size
  *this = this->operator*(A);
  return *this;
}

template<typename T>
MatrixBase<T>&
MatrixBase<T>::operator*=(const T& Value)
   /**
     MatrixBase multiplication THIS * Value
     @param Value :: Scalar to multiply matrix by
     @return *this
   */
{
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      V[i][j]*=Value;
  return *this;
}

template<typename T>
int
MatrixBase<T>::operator!=(const MatrixBase<T>& A) const
  /**
    Element by Element comparison
    @param A :: MatrixBase to check
    @return 1 :: on sucees
    @return 0 :: failoure
  */
{
  return (this->operator==(A)) ? 0 : 1;
}

template<typename T>
int
MatrixBase<T>::operator==(const MatrixBase<T>& A) const
  /**
    Element by element comparison within tolerance.
    Tolerance means that the value must be > tolerance
    and less than (diff/max)>tolerance

    Always returns 0 if the MatrixBase have different sizes
    @param A :: matrix to check.
    @return 1 on success
  */
{
  if (&A!=this)       // this == A == always true
    {
      if(A.nx!=nx || A.ny!=ny)
	return 0;

      for(int i=0;i<nx;i++)
	for(int j=0;j<ny;j++)
	  {
	    if (V[i][j]!=A.V[i][j])
	      return 0;
	  }
    }
  //this == this is true
  return 1;
}

template<typename T>
void
MatrixBase<T>::deleteMem()
  /**
    Deletes the memory held in matrix
  */
{
  if (V)
    {
      delete [] *V;
      delete [] V;
      V=0;
    }
  nx=0;
  ny=0;
  return;
}

template<typename T>
void
MatrixBase<T>::setMem(const int a,const int b)
  /**
    Sets the memory held in matrix
    @param a :: number of rows
    @param b :: number of columns
  */
{
  if (a==nx && b==ny)
    return;

  deleteMem();
  if (a<=0 || b<=0)
    return;

  nx=a;
  ny=b;
  if (nx*ny)
    {
      T* tmpX=new T[nx*ny];
      V=new T*[nx];
      for (int i=0;i<nx;i++)
	V[i]=tmpX + (i*ny);
    }
  return;
}


template<typename T>
std::vector<T>
MatrixBase<T>::Row(const int RowI) const
  /**
    Returns a specific row
    @param RowI :: Row index
    @return Vector of row
   */
{
  if (RowI<0 || RowI>=nx)
    throw Kernel::Exception::IndexError(RowI,nx,"MatrixBase::Row");
  std::vector<T> Out(V[RowI],V[RowI]+ny);
  return Out;
}

template<typename T>
std::vector<T>
MatrixBase<T>::Column(const int ColI) const
  /**
    Returns a specific column
    @param ColI :: Column index
    @return Vector of Column
   */
{
  if (ColI<0 || ColI>=ny)
    throw Kernel::Exception::IndexError(ColI,ny,"MatrixBase::Column");
  std::vector<T> Out(nx);
  for(int i=0;i<nx;i++)
    Out[i]=V[i][ColI];
  return Out;
}

template<typename T>
void
MatrixBase<T>::swapRows(const int RowI,const int RowJ)
  /**
    Swap rows I and J
    @param RowI :: row I to swap
    @param RowJ :: row J to swap
  */
{
  if (nx*ny && RowI<nx && RowJ<nx &&
      RowI!=RowJ)
    {
      for(int k=0;k<ny;k++)
        {
	  T tmp=V[RowI][k];
	  V[RowI][k]=V[RowJ][k];
	  V[RowJ][k]=tmp;
	}
    }
  return;
}

template<typename T>
void
MatrixBase<T>::swapCols(const int colI,const int colJ)
  /**
    Swap columns I and J
    @param colI :: col I to swap
    @param colJ :: col J to swap
  */
{
  if (nx*ny && colI<ny && colJ<ny &&
      colI!=colJ)
    {
      for(int k=0;k<nx;k++)
        {
	  T tmp=V[k][colI];
	  V[k][colI]=V[k][colJ];
	  V[k][colJ]=tmp;
	}
    }
  return;
}

template<typename T>
void
MatrixBase<T>::zeroMatrix()
  /**
    Zeros all elements of the matrix
  */
{
  if (nx*ny)
    for(int i=0;i<nx;i++)
      for(int j=0;j<ny;j++)
	V[i][j]=static_cast<T>(0);
  return;
}

template<typename T>
T
MatrixBase<T>::laplaceDeterminate() const
  /**
    This function calculates the determinate of a matrix.
    It uses the Cauche method of Det.
    There are some careful constructs to avoid that
    problem of matrix template object T not having a well
    defined 1.0 and an well defined 0.0.

    @return Determinate
  */
{
  if (nx*ny<=0 || nx!=ny)
    return T(0);
  if (nx==1)
    return V[0][0];
  if (nx==2)
    return V[0][0]*V[1][1]-V[0][1]*V[1][0];

  std::cerr<<"PreSet sum"<<std::endl;
  T Sum(V[0][0]);            // Avoids the problem of incomplete zero
  std::cerr<<"Set sum"<<std::endl;
  MatrixBase<T> M(*this,0,0);
  Sum*=M.laplaceDeterminate();

  for(int j=1;j<ny;j++)          // Loop over all columns:
    {
      MatrixBase<T> MX(*this,0,j);
      if (j % 2)
	Sum+=V[0][j]*MX.laplaceDeterminate();
      else
	Sum-=V[0][j]*MX.laplaceDeterminate();
    }
  return Sum;
}


template<typename T>
void
MatrixBase<T>::identityMatrix()
  /**
    Makes the matrix an idenity matrix.
    Zeros all the terms outside of the square
  */
{
  if (nx*ny)
    for(int i=0;i<nx;i++)
      for(int j=0;j<ny;j++)
	V[i][j]=(j==i) ? 1 : 0;
  return;
}

template<typename T>
MatrixBase<T>
MatrixBase<T>::fDiagonal(const std::vector<T>& Dvec) const
  /**
    Construct a matrix based on
    A * This, where A is made into a diagonal
    matrix.
    @param Dvec :: diagonal matrix (just centre points)
    @return a matrix multiplication
  */
{
  // Note:: nx,ny zeroed so setMem always works
  if (static_cast<int>(Dvec.size())!=nx)
    {
      std::ostringstream cx;
      cx<<"MatrixBase::fDiagonal Size: "<<Dvec.size()<<" "<<nx<<" "<<ny;
      throw std::runtime_error(cx.str());
    }
  MatrixBase<T> X(Dvec.size(),ny);
  for(int i=0;i<static_cast<int>(Dvec.size());i++)
    for(int j=0;j<ny;j++)
      X.V[i][j]=Dvec[i]*V[i][j];
  return X;
}

template<typename T>
MatrixBase<T>
MatrixBase<T>::bDiagonal(const std::vector<T>& Dvec) const
  /**
    Construct a matrix based on
    This * A, where A is made into a diagonal
    matrix.
    @param Dvec :: diagonal matrix (just centre points)
    @return the matrix result
  */
{
  // Note:: nx,ny zeroed so setMem always works
  if (static_cast<int>(Dvec.size())!=ny)
    {
      std::ostringstream cx;
      cx<<"Error MatrixBase::bDiaognal size:: "<<Dvec.size()
	<<" "<<nx<<" "<<ny;
      throw std::runtime_error(cx.str());
    }

  MatrixBase<T> X(nx,Dvec.size());
  for(int i=0;i<nx;i++)
    for(int j=0;j<static_cast<int>(Dvec.size());j++)
      X.V[i][j]=Dvec[j]*V[i][j];
  return X;
}

template<typename T>
MatrixBase<T>
MatrixBase<T>::Tprime() const
  /**
    Transpose the matrix :
    Has transpose for a square matrix case.
    @return M^T
  */
{
  if (!nx*ny)
    return *this;

  if (nx==ny)   // inplace transpose
    {
      MatrixBase<T> MT(*this);
      MT.Transpose();
      return MT;
    }

  // irregular matrix
  MatrixBase<T> MT(ny,nx);
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      MT.V[j][i]=V[i][j];

  return MT;
}

template<typename T>
MatrixBase<T>&
MatrixBase<T>::Transpose()
  /**
    Transpose the matrix :
    Has a inplace transpose for a square matrix case.
    @return this
  */
{
  if (!nx*ny)
    return *this;
  if (nx==ny)   // inplace transpose
    {
      for(int i=0;i<nx;i++)
	for(int j=i+1;j<ny;j++)
	  {
	    T tmp=V[i][j];
	    V[i][j]=V[j][i];
	    V[j][i]=tmp;
	  }
      return *this;
    }
  // irregular matrix
  // get some memory
  T* tmpX=new T[ny*nx];
  T** Vt=new T*[ny];
  for (int i=0;i<ny;i++)
    Vt[i]=tmpX + (i*nx);

  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      Vt[j][i]=V[i][j];
  // remove old memory
  const int tx=nx;
  const int ty=ny;
  deleteMem();  // resets nx,ny
  // replace memory
  V=Vt;
  nx=ty;
  ny=tx;

  return *this;
}

template<typename T>
T
MatrixBase<T>::compSum() const
  /**
    Add up each component sums for the matrix
    @return \f$ \sum_i \sum_j V_{ij}^2 \f$
   */
{
  T sum(0);
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      sum+=V[i][j]*V[i][j];
  return sum;
}

template<typename T>
std::vector<T>
MatrixBase<T>::Diagonal() const
  /**
    Returns the diagonal form as a vector
    @return Diagonal elements
  */
{
  const int Msize=(ny>nx) ? nx : ny;
  std::vector<T> Diag(Msize);
  for(int i=0;i<Msize;i++)
    Diag[i]=V[i][i];
  return Diag;
}

template<typename T>
T
MatrixBase<T>::Trace() const
  /**
    Calculates the trace of the matrix
    @return Trace of matrix
  */
{
  const int Msize=(ny>nx) ? nx : ny;
  T Trx(0);
  for(int i=0;i<Msize;i++)
    Trx+=V[i][i];
  return Trx;
}


template<typename T>
void
MatrixBase<T>::print() const
  /**
    Simple print out routine
   */
{
  write(std::cout,10);
  return;
}

template<typename T>
void
MatrixBase<T>::write(std::ostream& Fh,const int blockCnt) const
  /**
    Write out function for blocks of 10 Columns
    @param Fh :: file stream for output
    @param blockCnt :: number of columns per line (0 == full)
  */
{

  std::ios::fmtflags oldFlags=Fh.flags();
  Fh.setf(std::ios::floatfield,std::ios::scientific);
  const int blockNumber((blockCnt>0) ? blockCnt : ny);
  int BCnt(0);

  do
    {
      const int ACnt=BCnt;
      BCnt+=blockNumber;
      if (BCnt>ny)
	BCnt=ny;

      if (ACnt)
	Fh<<" ----- "<<ACnt<<" "<<BCnt<<" ------ "<<std::endl;
      for(int i=0;i<nx;i++)
        {
	  for(int j=ACnt;j<BCnt;j++)
	    Fh<<std::setw(10)<<V[i][j]<<"  ";
	  Fh<<std::endl;
	}
    } while(BCnt<ny);

  Fh.flags(oldFlags);
  return;
}

template<typename T>
void
MatrixBase<T>::writeGrid(std::ostream& FX) const
  /**
    Write out function for blocks:
    @param FX :: file stream for output
  */
{
  if (nx*ny<1) return;
  // need a list of longest strings [for each column]:
  std::vector<int> LStr(ny);
  fill(LStr.begin(),LStr.end(),0);

  // a matrix of strings:
  boost::multi_array<std::string,2> MStr(boost::extents[nx][ny]);

  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      {
	std::stringstream cx;
	cx<<V[i][j];
	const int len(cx.str().length());
	if (LStr[j]<len)
	  LStr[j]=len;
	MStr[i][j]=cx.str();
      }

  // WRITE PART:
  for(int i=0;i<nx;i++)
    {
      for(int j=0;j<ny;j++)
        {
	  const int Fpad(LStr[j]-MStr[i][j].length());
	  const int Lpad(2+Fpad/2);
	  const int Rpad(2+Lpad + (Fpad % 2));
	  FX<<std::string(Lpad,' ')<<MStr[i][j]<<std::string(Rpad,' ');
	}
      FX<<std::endl;
    }
  return;
}

template<typename T>
std::string
MatrixBase<T>::str(const int spx) const
  /**
    Convert the matrix into a simple linear string expression
    @param spx :: The precision to use in std::setprecision
    @return String value of output
  */
{
  std::ostringstream cx;
  for(int i=0;i<nx;i++)
    for(int j=0;j<ny;j++)
      {
	if (spx)
	  cx<<std::setprecision(spx)<<V[i][j]<<" ";
	else
	  cx<<V[i][j]<<" ";
      }
  return cx.str();
}

///@cond
template class MatrixBase<double>;
template class MatrixBase<int>;
template class MatrixBase<mathLevel::PolyVar<1> >;
template class MatrixBase<mathLevel::PolyVar<2> >;
template class MatrixBase<mathLevel::PolyVar<3> >;

template std::ostream& operator<<(std::ostream&,const MatrixBase<double>&);
template std::ostream& operator<<(std::ostream&,const MatrixBase<int>&);
template std::ostream& operator<<(std::ostream&,
				  const MatrixBase<mathLevel::PolyVar<1> >&);
template std::ostream& operator<<(std::ostream&,
				  const MatrixBase<mathLevel::PolyVar<2> >&);
template std::ostream& operator<<(std::ostream&,
				  const MatrixBase<mathLevel::PolyVar<3> >&);
///@endcond

}  // NAMESPACE Geometry

}  // NAMESPACE Mantid
