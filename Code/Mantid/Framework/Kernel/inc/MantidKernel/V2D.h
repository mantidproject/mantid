#ifndef MANTID_KERNEL_V2D_H_
#define MANTID_KERNEL_V2D_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <ostream>

namespace Mantid
{
  namespace Kernel
  {
    class V3D;

    /**
    Implements a 2-dimensional vector embedded in a 3D space, i.e.
    such that the cross product of two 2D vectors is a 3D vector in the 
    Z direction

    @author Martyn Gigg, Tessella plc

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MANTID_KERNEL_DLL V2D
    {
    public:
      /**
       * Default constructor. It puts the vector at the origin
       */
      inline V2D() : m_x(0.0), m_y(0.0) 
      {
      }
      /**
       * Constructor taking an x and y value.
       */
      inline V2D(const double x, const double y) 
        : m_x(x), m_y(y)
      {
      }
      /**
       * X position
       * @returns The X position
       */
      inline const double& X() const { return m_x; }
      /**
       * Y position
       * @returns The Y position
       */
      inline const double& Y() const { return m_y; }
      /// Index access.
      const double& operator[](const size_t index) const;

      ///@name Arithmetic operations
      ///@{
      /// Sum this and the rhs
      V2D operator+(const V2D &rhs) const;
      /// Increment this vector by rhs
      V2D & operator+=(const V2D &rhs);
      /// Subtract this and the rhs
      V2D operator-(const V2D &rhs) const;
      /// Decrement this by rhs
      V2D & operator-=(const V2D &rhs);
      /// Scale and return
      V2D operator*(const double factor) const;
      /// Scale this
      V2D& operator*=(const double factor);

      ///@}

      ///@name Comparison operators
      ///@{
      ///Equality operator
      bool operator==(const V2D& rhs) const;
      ///Inequality operator
      bool operator!=(const V2D& rhs) const;
      ///@}

      /// Make a normalized vector (return norm value)
      double normalize();
      /// Compute the norm
      double norm() const;
      /// Compute the square of the norm
      double norm2() const;
      // Scalar product
      double scalar_prod(const V2D &other) const;
      // Cross product
      V3D cross_prod(const V2D &other) const;
      /// Distance (R) between two points defined as vectors
      double distance(const V2D &other) const;
      // Angle between this and another vector
      double angle(const V2D &other) const;

    private:
      /// X values
      double m_x;
      /// Y values
      double m_y;
    };

    // Overload operator <<
    MANTID_KERNEL_DLL std::ostream& operator<<(std::ostream&, const V2D&);
    
  } //namespace Kernel
} //namespace Mantid

#endif // MANTID_KERNEL_V2D_H_
