#ifndef MatrixBase_h
#define MatrixBase_h
#include "MantidKernel/System.h"

namespace Mantid
{

namespace mathLevel
{
  template<int index>
  class PolyVar;
}

/**
   \brief Classes for geometric handlers
   \version 1.0
   \author S. Ansell
   \date February 2006

*/
namespace Geometry
{

/**
  \class MatrixBase
  \brief Numerical MatrixBase class
  \version 1.0
  \author S. Ansell
  \date August 2005

  Holds a matrix of variable type
  and size. Should work for real and
  complex objects. Carries out eigenvalue
  and inversion if the matrix is square
*/
class Vec3D;


template<typename T>
class DLLExport MatrixBase
{
 private:

  int nx;      ///< Number of rows    (x coordinate)
  int ny;      ///< Number of columns (y coordinate)

  T** V;                      ///< Raw data

  void deleteMem();           ///< Helper function to delete memory

 public:

  MatrixBase(int const =0,int const =0);
  MatrixBase(const std::vector<T>&,const std::vector<T>&);
  MatrixBase(const MatrixBase<T>&);
  MatrixBase(const MatrixBase<T>&,const int nrow,const int ncol);
  MatrixBase<T>& operator=(const MatrixBase<T>&);
  ~MatrixBase();

  const T* operator[](const int A) const { return V[A]; } ///< Ptr accessor
  T* operator[](const int A) { return V[A]; }             ///< Ptr accessor

  MatrixBase<T>& operator+=(const MatrixBase<T>&);       ///< Basic addition operator
  MatrixBase<T> operator+(const MatrixBase<T>&);         ///< Basic addition operator

  MatrixBase<T>& operator-=(const MatrixBase<T>&);       ///< Basic subtraction operator
  MatrixBase<T> operator-(const MatrixBase<T>&);         ///< Basic subtraction operator

  MatrixBase<T> operator*(const MatrixBase<T>&) const;    ///< Basic matrix multiply
  std::vector<T> operator*(const std::vector<T>&) const; ///< Multiply M*Vec
  //Vec3D operator*(const Vec3D&) const; ///< Multiply M*Vec
  MatrixBase<T> operator*(const T&) const;              ///< Multiply by constant

  MatrixBase<T>& operator*=(const MatrixBase<T>&);            ///< Basic matrix multipy
  MatrixBase<T>& operator*=(const T&);                 ///< Multiply by constant

  int operator==(const MatrixBase<T>&) const;
  int operator!=(const MatrixBase<T>&) const;
  /// Item access
  T item(const int a,const int b) const { return V[a][b]; }

  void print() const;
  void write(std::ostream&,int const =0) const;
  void writeGrid(std::ostream&) const;
  std::string str(const int spx =6) const;

  void zeroMatrix();
  void identityMatrix();
  //void normVert();         ///< Vertical normalisation
  T Trace() const;         ///< Trace of the matrix

  std::vector<T> Diagonal() const;                  ///< Returns a vector of the diagonal
  MatrixBase<T> fDiagonal(const std::vector<T>&) const;    ///< Forward multiply  D*this
  MatrixBase<T> bDiagonal(const std::vector<T>&) const;    ///< Backward multiply this*D
  std::vector<T> Row(int const) const;
  std::vector<T> Column(int const) const;

  void setMem(int const,int const);
  /// Size values
  std::pair<int,int> size() const { return std::pair<int,int>(nx,ny); }
  int Ssize() const { return (nx>ny) ? ny : nx; }  ///< Smalles size

  void swapRows(int const,int const);        ///< Swap rows (first V index)
  void swapCols(int const,int const);        ///< Swap cols (second V index)

  //T Invert();                           ///< LU inversion routine
  /// Polynomanal and inversion by Faddeev method.
  //std::vector<T> Faddeev(MatrixBase<T>&);
  //void averSymmetric();                          ///< make MatrixBase symmetric
  //int Diagonalise(MatrixBase<T>&,MatrixBase<T>&) const;  ///< (only Symmetric matrix)
  //void sortEigen(MatrixBase<T>&);                    ///< Sort eigenvectors

  MatrixBase<T>& Transpose();                        ///< Transpose the matrix
  MatrixBase<T> Tprime() const;                      ///< Transpose the matrix

  T laplaceDeterminate() const;
  T compSum() const;

};

template<typename T>
std::ostream&
operator<<(std::ostream&,const Geometry::MatrixBase<T>&);

template<int index>
std::ostream&
operator<<(std::ostream&,
	   const Geometry::MatrixBase<mathLevel::PolyVar<index> >&);

}  // NAMESPACE  Geometry


}

// template<typename X>
// std::ostream& operator<<(std::ostream&,const Geometry::MatrixBase<X>&);

#endif














