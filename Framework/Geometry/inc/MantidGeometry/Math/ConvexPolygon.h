// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_CONVEXPOLYGON_H_
#define MANTID_GEOMETRY_CONVEXPOLYGON_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V2D.h"
#include <iosfwd>
#include <vector>

namespace Mantid {
namespace Geometry {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
class PolygonEdge;

/**
An implementation of a convex polygon. It contains a list of vertices that
make up a convex polygon and the list is assumed to be ordered in a
clockwise manner and the polygon is assumed to be closed.

A polygon is convex if:
<UL>
<LI>Every internal angle is less than or equal to 180 degrees.</LI>
<LI>Every line segment between two vertices remains inside or on the boundary of
the polygon.</LI>
</UL>
*/
class MANTID_GEOMETRY_DLL ConvexPolygon {

public:
  /// Type of the point list
  using Vertices = std::vector<Kernel::V2D>;

  //-----------------------------------------------------------------
  // Forward directional iterator inner class
  //-----------------------------------------------------------------
  class MANTID_GEOMETRY_DLL Iterator {
  public:
    /// Constructor
    Iterator(const ConvexPolygon &polygon);

    /// Dereference operator
    const Kernel::V2D &operator*() const;
    /// Prefix increment operator
    void operator++();
    /// Create a directed edge between this and the next point
    PolygonEdge edge() const;

  private:
    /// Compute the next index
    size_t nextIndex() const;

    const ConvexPolygon &m_polygon;
    size_t m_index;
  };

  //-----------------------------------------------------------------
  // ConvexPolygon class
  //-----------------------------------------------------------------

  /// Default constructor
  ConvexPolygon();
  /// Construct a polygon from a collection of points
  ConvexPolygon(const Vertices &vertices);
  /// Destructor
  virtual ~ConvexPolygon() = default;
  ConvexPolygon(ConvexPolygon &&) = default;
  ConvexPolygon &operator=(ConvexPolygon &&) = default;
  ConvexPolygon(const ConvexPolygon &) = default;
  ConvexPolygon &operator=(const ConvexPolygon &) = default;

  /// Check if polygon is valid
  bool isValid() const;
  /// Clears all points
  void clear();
  /// Insert a new vertex. The point is assumed that it forms the next point in
  /// a clockwise-sense around the shape
  void insert(const Kernel::V2D &pt);
  /// Insert a new vertex based on x,y values
  void insert(double x, double y);
  /// Index access.
  virtual const Kernel::V2D &operator[](const size_t index) const;
  /// Bounds-checked index access
  virtual const Kernel::V2D &at(const size_t index) const;
  /// Return the number of vertices
  virtual size_t npoints() const;
  /// Is a point inside this polygon
  virtual bool contains(const Kernel::V2D &point) const;
  /// Is a the given polygon completely encosed by this one
  virtual bool contains(const ConvexPolygon &poly) const;
  /// Compute the area of the polygon using triangulation
  virtual double area() const;
  /// Compute the 'determinant' of the points
  virtual double determinant() const;
  /// Return the lowest X value in the polygon
  virtual double minX() const;
  /// Return the largest X value in the polygon
  virtual double maxX() const;
  /// Return the lowest Y value in the polygon
  virtual double minY() const;
  /// Return the largest Y value in the polygon
  virtual double maxY() const;
  /// Return a new Polygon based on the current type
  virtual ConvexPolygon toPoly() const;

private:
  /// Setup the meta-data
  void setup();
  /// Compute the area of a triangle given by 3 points
  double triangleArea(const Kernel::V2D &a, const Kernel::V2D &b,
                      const Kernel::V2D &c) const;
  /// Lowest X value
  double m_minX;
  /// Highest X value
  double m_maxX;
  /// Lowest Y value
  double m_minY;
  /// Highest Y value
  double m_maxY;
  // Points of the polygon
  Vertices m_vertices;
};

/// Print a polygon to a stream
MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &os,
                                             const ConvexPolygon &polygon);

} // namespace Geometry
} // namespace Mantid

#endif // MANTID_GEOMETRY_CONVEXPOLYGON_H_
