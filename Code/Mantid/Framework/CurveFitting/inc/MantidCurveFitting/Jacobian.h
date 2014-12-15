#ifndef MANTID_CURVEFITTING_GSLFUNCTIONS_H_
#define MANTID_CURVEFITTING_GSLFUNCTIONS_H_

#include "MantidAPI/Jacobian.h"
#include <gsl/gsl_matrix.h>

#include <vector>
#include <stdexcept>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    An implementation of Jacobian using std::vector.

    @author Roman Tolchenov
    @date 17/02/2012

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
  class Jacobian: public API::Jacobian
  {
    /// Number of data points
    size_t m_ny;
    /// Number of parameters in a function (== IFunction::nParams())
    size_t m_np;
    /// Storage for the derivatives
    std::vector<double> m_data;
  public:
    /// Constructor.
    /// @param ny :: Number of data points
    /// @param np :: Number of parameters
    Jacobian(size_t ny,size_t np): m_ny(ny), m_np(np)
    {
      m_data.resize(ny * np, 0.0);
    }
    /// overwrite base method
    /// @param value :: the value
    /// @param iP :: the index of the parameter
    ///  @throw runtime_error Thrown if column of Jacobian to add number to does not exist
    void addNumberToColumn(const double& value, const size_t& iP)
    {
      if (iP < m_np )
      {
        // add penalty to first and last point and every 10th point in between
        m_data[iP] += value;
        m_data[(m_ny - 1) * m_np + iP] += value;
        for (size_t iY = 9; iY < m_ny; iY+=10)
          m_data[iY*m_np + iP] += value;
      }
      else
      {
        throw std::runtime_error("Try to add number to column of Jacobian matrix which does not exist.");
      }   
    }
    /// overwrite base method
    void set(size_t iY, size_t iP, double value)
    {
      if (iY >= m_ny)
      {
        throw std::out_of_range("Data index in Jacobian is out of range");
      }
      if (iP >= m_np)
      {
        throw std::out_of_range("Parameter index in Jacobian is out of range");
      }
      m_data[iY*m_np + iP] = value;
    }
    /// overwrite base method
    double get(size_t iY, size_t iP)
    {
      if (iY >= m_ny)
      {
        throw std::out_of_range("Data index in Jacobian is out of range");
      }
      if (iP >= m_np)
      {
        throw std::out_of_range("Parameter index in Jacobian is out of range");
      }
      return m_data[iY*m_np + iP];
    }
  };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLFUNCTIONS_H_*/
