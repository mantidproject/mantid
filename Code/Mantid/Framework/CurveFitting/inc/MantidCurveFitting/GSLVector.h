#ifndef MANTID_CURVEFITTING_GSLVECTOR_H_
#define MANTID_CURVEFITTING_GSLVECTOR_H_

#include <gsl/gsl_vector.h>

#include <stdexcept>

namespace Mantid
{
  namespace CurveFitting
  {
    /**

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
  class GSLVector
  {
    /// The pointer to the GSL's internal jacobian matrix
    gsl_vector * m_vector;
    
  public:
    /// Constructor
    GSLVector(const size_t n)
    {
      m_vector = gsl_vector_alloc(n);
    }

    /// Destructor.
    ~GSLVector()
    {
      gsl_vector_free(m_vector);
    }

    /// Get the pointer to the GSL vector
    gsl_vector * gsl(){return m_vector;}

    void resize(const size_t n)
    {
      gsl_vector_free(m_vector);
      m_vector = gsl_vector_alloc(n);
    }

    /// Size of the vector
    size_t size() const {return m_vector->size;}

    /// set an element
    void set(size_t i, double value)
    {
      if (i < m_vector->size) gsl_vector_set(m_vector,i,value);
      else
      {
        throw std::out_of_range("GSLVector index is out of range.");
      }
    }
    /// get an element
    double get(size_t i)
    {
      if (i < m_vector->size) return gsl_vector_get(m_vector,i);
      throw std::out_of_range("GSLVector index is out of range.");
    }
  };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLVECTOR_H_*/
