#ifndef MANTID_GEOMETRY_MATRIX_H_
#define MANTID_GEOMETRY_MATRIX_H_

#include "MantidKernel/System.h"
#include <vector>

namespace Mantid
{

  namespace Geometry
  {
    //-------------------------------------------------------------------------
    // Forward declarations
    //-------------------------------------------------------------------------
    class V3D;

    /**  Numerical Matrix class.     Holds a matrix of variable type and size. 
    Should work for real and complex objects. Carries out eigenvalue
    and inversion if the matrix is square
    
    Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
    template<typename T>
    class DLLExport Matrix
    {
    private:

      size_t nx;      ///< Number of rows    (x coordinate)
      size_t ny;      ///< Number of columns (y coordinate)

      T** V;                      ///< Raw data
      void deleteMem();           ///< Helper function to delete memory
      void lubcmp(int*,int&);     ///< starts inversion process
      void lubksb(int const*,double*);
      void rotate(double const,double const,
        int const,int const,int const,int const);

    public:

      Matrix(const size_t nrow = 0, const size_t ncol = 0, bool const makeIdentity=false);
      Matrix(const std::vector<T>&,const std::vector<T>&);
      Matrix(const Matrix<T>&,const size_t nrow,const size_t ncol);

      Matrix(const Matrix<T>&);
      Matrix<T>& operator=(const Matrix<T>&);
      ~Matrix();

      /// const Array accessor
      const T* operator[](const size_t A) const { return V[A]; }
      /// Array accessor
      T* operator[](const size_t A) { return V[A]; }

      Matrix<T>& operator+=(const Matrix<T>&);       ///< Basic addition operator
      Matrix<T> operator+(const Matrix<T>&);         ///< Basic addition operator

      Matrix<T>& operator-=(const Matrix<T>&);       ///< Basic subtraction operator
      Matrix<T> operator-(const Matrix<T>&);         ///< Basic subtraction operator

      Matrix<T> operator*(const Matrix<T>&) const;    ///< Basic matrix multiply
      std::vector<T> operator*(const std::vector<T>&) const; ///< Multiply M*Vec
      V3D operator*(const V3D&) const; ///< Multiply M*Vec
      Matrix<T> operator*(const T&) const;              ///< Multiply by constant

      Matrix<T>& operator*=(const Matrix<T>&);            ///< Basic matrix multipy
      Matrix<T>& operator*=(const T&);                 ///< Multiply by constant
      Matrix<T>& operator/=(const T&);                 ///< Divide by constant

      bool operator!=(const Matrix<T>&) const;
      bool operator==(const Matrix<T>&) const;
      bool equals(const Matrix<T>& A, const double Tolerance) const;
      T item(const int a,const int b) const { return V[a][b]; }   ///< disallows access

      void print() const;
      void write(std::ostream&,int const =0) const;
      std::string str() const;

      // returns this matrix in 1D vector representation
      std::vector<T> get_vector()const;
      // explicit conversion into the vector
      operator std::vector<T>()const{std::vector<T> tmp=this->get_vector(); return tmp;}
      //
      void setColumn(const size_t nCol,const std::vector<T> &newColumn);
      void setRow(const size_t nRow,const std::vector<T> &newRow);
      void zeroMatrix();      ///< Set the matrix to zero
      void identityMatrix();
      void normVert();         ///< Vertical normalisation
      T Trace() const;         ///< Trace of the matrix

      std::vector<T> Diagonal() const;                  ///< Returns a vector of the diagonal
      Matrix<T> fDiagonal(const std::vector<T>&) const;    ///< Forward multiply  D*this
      Matrix<T> bDiagonal(const std::vector<T>&) const;    ///< Backward multiply this*D

      void setMem(const size_t, const size_t);

      /// Access matrix sizes
      std::pair<size_t,size_t> size() const { return std::pair<size_t,size_t>(nx,ny); }

      /// Return the number of rows in the matrix
      size_t numRows() const { return nx; }

      /// Return the number of columns in the matrix
      size_t numCols() const { return ny; }

      /// Return the largest matrix size
      size_t Ssize() const { return (nx>ny) ? ny : nx; }

      void swapRows(const size_t,const size_t);        ///< Swap rows (first V index)
      void swapCols(const size_t,const size_t);        ///< Swap cols (second V index)

      T Invert();                           ///< LU inversion routine
      std::vector<T> Faddeev(Matrix<T>&);      ///< Polynomanal and inversion by Faddeev method.
      void averSymmetric();                 ///< make Matrix symmetric
      int Diagonalise(Matrix<T>&,Matrix<T>&) const;  ///< (only Symmetric matrix)
      void sortEigen(Matrix<T>&);                    ///< Sort eigenvectors
      Matrix<T> Tprime() const;                      ///< Transpose the matrix
      Matrix<T>& Transpose();                        ///< Transpose the matrix

      T factor();                       ///< Calculate the factor
      T determinant() const;            ///< Calculate the determinant

      int GaussJordan(Matrix<T>&);      ///< Create a Gauss-Jordan Invertion
      T compSum() const;

    };

    template<typename T>
    DLLExport std::ostream&
      operator<<(std::ostream&,const Geometry::Matrix<T>&);
    typedef Mantid::Geometry::Matrix<double> MantidMat;
  }
}  
#endif //MANTID_GEOMETRY_MATRIX_H_
