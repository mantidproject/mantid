#ifndef MANTID_CURVEFITTING_GSLVECTOR_H_
#define MANTID_CURVEFITTING_GSLVECTOR_H_

#include <gsl/gsl_vector.h>

#include <stdexcept>
#include <sstream>
#include <vector>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    A wrapper around gsl_vector.

    @author Roman Tolchenov, Tessella plc
    @date 24/02/2012

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class GSLVector
  {
    /// The pointer to the GSL vector
    gsl_vector * m_vector;
    
  public:
    /// Constructor
    GSLVector():m_vector(NULL){}

    /// Constructor
    /// @param n :: The length of the vector.
    GSLVector(const size_t n)
    {
      m_vector = gsl_vector_alloc(n);
    }

    /// Construct from a std vector
    /// @param v :: A std vector.
    GSLVector(const std::vector<double>& v)
    {
      m_vector = gsl_vector_alloc( v.size() );
      for(size_t i = 0; i < v.size(); ++i)
      {
          set( i, v[i] );
      }
    }

    /// Copy constructor.
    /// @param v :: The other vector
    GSLVector(const GSLVector& v)
    {
      m_vector = gsl_vector_alloc(v.size());
      gsl_vector_memcpy(m_vector, v.gsl());
    }

    /// Copy assignment operator
    /// @param v :: The other vector
    GSLVector& operator=(const GSLVector& v)
    {
      if (m_vector && size() != v.size())
      {
        gsl_vector_free(m_vector);
        m_vector = NULL;
      }
      if (!m_vector)
      {
        m_vector = gsl_vector_alloc(v.size());
      }
      gsl_vector_memcpy(m_vector, v.gsl());
      return *this;
    }

    /// Destructor.
    ~GSLVector()
    {
      if (m_vector)
      {
        gsl_vector_free(m_vector);
      }
    }

    /// Get the pointer to the GSL vector
    gsl_vector * gsl(){return m_vector;}

    /// Get the pointer to the GSL vector
    const gsl_vector * gsl() const {return m_vector;}

    /// Resize the vector
    /// @param n :: The new length
    void resize(const size_t n)
    {
      if ( m_vector )
      {
        gsl_vector_free(m_vector);
      }
      m_vector = gsl_vector_alloc(n);
    }

    /// Size of the vector
    size_t size() const {return m_vector? m_vector->size : 0;}

    /// set an element
    /// @param i :: The element index
    /// @param value :: The new value
    void set(size_t i, double value)
    {
      if (i < m_vector->size) gsl_vector_set(m_vector,i,value);
      else
      {
        std::stringstream errmsg;
        errmsg << "GSLVector index = " << i << " is out of range = "
               << m_vector->size << " in GSLVector.set()";
        throw std::out_of_range(errmsg.str());
      }
    }
    /// get an element
    /// @param i :: The element index
    double get(size_t i) const
    {
      if (i < m_vector->size) return gsl_vector_get(m_vector,i);

      std::stringstream errmsg;
      errmsg << "GSLVector index = " << i << " is out of range = "
             << m_vector->size << " in GSLVector.get()";
      throw std::out_of_range(errmsg.str());
    }

    // Set all elements to zero
    void zero()
    {
      if ( m_vector )
      {
        gsl_vector_set_zero( m_vector );
      }
    }

    /// Add a vector
    /// @param v :: The other vector
    GSLVector& operator+=(const GSLVector& v)
    {
      if ( m_vector )
      {
        gsl_vector_add(m_vector, v.gsl());
      }
      return *this;
    }

    /// Subtract a vector
    /// @param v :: The other vector
    GSLVector& operator-=(const GSLVector& v)
    {
      if ( m_vector )
      {
        gsl_vector_sub(m_vector, v.gsl());
      }
      return *this;
    }

    /// Multiply by a number
    /// @param d :: The number
    GSLVector& operator*=(const double d)
    {
      if ( m_vector )
      {
        gsl_vector_scale(m_vector, d);
      }
      return *this;
    }

  };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLVECTOR_H_*/
