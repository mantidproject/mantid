#ifndef MANTID_CURVEFITTING_GSLMATRIX_H_
#define MANTID_CURVEFITTING_GSLMATRIX_H_

#include <gsl/gsl_matrix.h>

#include <vector>
#include <stdexcept>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    An implementation of Jacobian using gsl_matrix.

    @author Roman Tolchenov, Tessella plc
    @date 24/02/2012

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
  class GSLMatrix
  {
    /// The pointer to the GSL's internal jacobian matrix
    gsl_matrix * m_matrix;
    
  public:
    /// Constructor
    GSLMatrix():m_matrix(NULL){}
    /// Constructor
    GSLMatrix(const size_t nx, const size_t ny)
    {
      m_matrix = gsl_matrix_alloc(nx,ny);
    }

    /// Copy constructor
    GSLMatrix(const GSLMatrix& M)
    {
      m_matrix = gsl_matrix_alloc(M.size1(),M.size2());
      gsl_matrix_memcpy(m_matrix,M.gsl());
    }

    /// Destructor.
    ~GSLMatrix()
    {
      if (m_matrix)
      {
        gsl_matrix_free(m_matrix);
      }
    }

    /// Get the pointer to the GSL matrix
    gsl_matrix * gsl(){return m_matrix;}

    /// Get the const pointer to the GSL matrix
    const gsl_matrix * gsl() const {return m_matrix;}

    void resize(const size_t nx, const size_t ny)
    {
      gsl_matrix_free(m_matrix);
      m_matrix = gsl_matrix_alloc(nx,ny);
    }

    /// First size of the matrix
    size_t size1() const {return m_matrix? m_matrix->size1 : 0;}

    /// Second size of the matrix
    size_t size2() const {return m_matrix? m_matrix->size2 : 0;}

    /// set an element
    void set(size_t i, size_t j, double value)
    {
      if (i < m_matrix->size1 && j < m_matrix->size2) gsl_matrix_set(m_matrix,i,j,value);
      else
      {
        throw std::out_of_range("GSLMatrix indices are out of range.");
      }
    }
    /// get an element
    double get(size_t i, size_t j) const
    {
      if (i < m_matrix->size1 && j < m_matrix->size2) return gsl_matrix_get(m_matrix,i,j);
      throw std::out_of_range("GSLMatrix indices are out of range.");
    }

    // Set this matrix to identity matrix
    void identity()
    {
      gsl_matrix_set_identity( m_matrix );
    }

    // Set all elements to zero
    void zero()
    {
      gsl_matrix_set_zero( m_matrix );
    }

    // add a matrix to this
    GSLMatrix& operator+=(const GSLMatrix& M)
    {
      gsl_matrix_add( m_matrix, M.gsl() );
      return *this;
    }

    // add a constant to this matrix
    GSLMatrix& operator+=(const double& d)
    {
      gsl_matrix_add_constant( m_matrix, d );
      return *this;
    }

    // subtract a matrix from this
    GSLMatrix& operator-=(const GSLMatrix& M)
    {
      gsl_matrix_sub( m_matrix, M.gsl() );
      return *this;
    }

    // multiply this matrix by a number
    GSLMatrix& operator*=(const double& d)
    {
      gsl_matrix_scale( m_matrix, d );
      return *this;
    }
  };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLMATRIX_H_*/
