#ifndef MANTID_GEOMETRY_CONVEXPOLYGON_H_
#define MANTID_GEOMETRY_CONVEXPOLYGON_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Math/Vertex2DList.h"

namespace Mantid
{
  //---------------------------------------------------------------------------
  // Forward declarations
  //---------------------------------------------------------------------------

  namespace Geometry
  {

    /** 
    An implementation of a convex polygon. It contains a list of vertices that
    make up a convex polygon and the list is assumed to be ordered in an
    anti-clockwise manner.

    A polygon is convex if:
    <UL>
    <LI>Every internal angle is less than or equal to 180 degrees.</LI>
    <LI>Every line segment between two vertices remains inside or on the boundary of the polygon.</LI>
    </UL>

    @author Martyn Gigg, Tessella plc

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class MANTID_GEOMETRY_DLL ConvexPolygon
    {
    public:
      /// Construct a polygon with a collection of vertices
      ConvexPolygon(const Vertex2DList & vertices);
      /// Construct a rectangle as these will be quite common
      ConvexPolygon(const double x_lower, const double x_upper, 
                    const double y_lower, const double y_upper);

      /// Index access.
      const Kernel::V2D& operator[](const size_t index) const;
      /// Return the number of vertices
      inline size_t numVertices() const { return m_vertices.size(); }
      /// Compute the area of the polygon using triangulation
      double area() const;
      /// Compute the 'determinant' of the points
      double determinant() const;
      /// Compute the orientation
      int orientation() const;

    private:
      /// Default constructor
      ConvexPolygon();
      /// Test if the set of vertices set is valid
      void validate() const;
      /// Compute the area of a triangle given by 3 vertices
      double triangleArea(const Kernel::V2D & a, const Kernel::V2D & b, 
                          const Kernel::V2D & c) const;
      
      /// The collection of vertices
      Vertex2DList m_vertices;
    };

    /// Print a polygon to a stream
    std::ostream & operator<<(std::ostream & os, const ConvexPolygon & polygon);

  } //namespace Geometry
} //namespace Mantid


#endif //MANTID_GEOMETRY_CONVEXPOLYGON_H_
