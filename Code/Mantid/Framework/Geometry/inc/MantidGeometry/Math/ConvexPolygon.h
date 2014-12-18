#ifndef MANTID_GEOMETRY_CONVEXPOLYGON_H_
#define MANTID_GEOMETRY_CONVEXPOLYGON_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Math/PolygonEdge.h"
#include <vector>

namespace Mantid {
namespace Geometry {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
class Vertex2D;

//---------------------------------------------------------------------------
// Typedefs
//---------------------------------------------------------------------------

/**
An implementation of a convex polygon. It contains a list of vertices that
make up a convex polygon and the list is assumed to be ordered in an
anti-clockwise manner.

A polygon is convex if:
<UL>
<LI>Every internal angle is less than or equal to 180 degrees.</LI>
<LI>Every line segment between two vertices remains inside or on the boundary of
the polygon.</LI>
</UL>

@author Martyn Gigg, Tessella plc

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class MANTID_GEOMETRY_DLL ConvexPolygon {

public:
  /// Construct a polygon with a head vertex
  ConvexPolygon(Vertex2D *head);
  /// Construct a rectangle as these will be quite common
  ConvexPolygon(const double x_lower, const double x_upper,
                const double y_lower, const double y_upper);
  /// Copy constructor
  ConvexPolygon(const ConvexPolygon &rhs);
  /// Destructor
  virtual ~ConvexPolygon();
  /// Return a pointer to the head vertex
  virtual inline const Vertex2D *head() const { return m_head; }
  /// Index access.
  virtual const Kernel::V2D &operator[](const size_t index) const;
  /// Return the number of vertices
  inline size_t numVertices() const { return m_numVertices; }
  /// Is a point inside this polygon
  virtual bool contains(const Kernel::V2D &point) const;
  /// Is a the given polygon completely encosed by this one
  virtual bool contains(const ConvexPolygon &poly) const;
  /// Compute the area of the polygon using triangulation
  virtual double area() const;
  /// Compute the 'determinant' of the points
  virtual double determinant() const;
  /// Return the lowest X value in the polygon
  double smallestX() const;
  /// Return the largest X value in the polygon
  double largestX() const;
  /// Return the lowest Y value in the polygon
  double smallestY() const;
  /// Return the largest Y value in the polygon
  double largestY() const;

protected:
  /// Default constructor
  ConvexPolygon() : m_numVertices(0), m_head(NULL) {}
  /// Setup the meta-data
  void setup();
  /// The size of the polygon
  size_t m_numVertices;
  /// Head vertex
  Vertex2D *m_head;
  /// Lowest X value
  double m_lowestX;
  /// Highest X value
  double m_highestX;
  /// Lowest Y value
  double m_lowestY;
  /// Highest Y value
  double m_highestY;

private:
  /// Test if a list of vertices is valid
  void validate(const Vertex2D *head) const;
  /// Compute the area of a triangle given by 3 points
  double triangleArea(const Kernel::V2D &a, const Kernel::V2D &b,
                      const Kernel::V2D &c) const;
};

/// Print a polygon to a stream
MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &os,
                                             const ConvexPolygon &polygon);

} // namespace Geometry
} // namespace Mantid

#endif // MANTID_GEOMETRY_CONVEXPOLYGON_H_
