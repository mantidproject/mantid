// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V2D.h"

namespace Mantid {
namespace Geometry {
/** PolygonEdge
  Defines a directed edge between two points on a polygon

  @author Martyn Gigg
  @date 2011-07-08
*/
class DLLExport PolygonEdge {
public:
  /// Defines the orientation with respect to another edge
  enum Orientation {
    Collinear,  /**< Edges lie on the same line */
    Parallel,   /**< Edges point in the same direction */
    Skew,       /**< Edges are at an angle to each other */
    SkewCross,  /**< Edges are at an angle and intersect */
    SkewNoCross /**< Edges are at an angle and do not intersect */
  };

  /// Constructor with starting and ending points
  PolygonEdge(const Kernel::V2D &start, const Kernel::V2D &end);
  /// Access the start point
  inline const Kernel::V2D &start() const { return m_start; }
  /// Access the end point
  inline const Kernel::V2D &end() const { return m_end; }
  /// Return the direction
  inline const Kernel::V2D &direction() const { return m_dir; }
  /// Create a point a given fraction along this edge
  Kernel::V2D point(const double fraction) const;

private:
  /// Default constructor
  PolygonEdge();

  /// Origin point
  const Kernel::V2D m_start;
  /// Destination point
  const Kernel::V2D m_end;
  /// Direction vector
  const Kernel::V2D m_dir;
};

/// Enumeration for point type w.r.t an edge
enum PointClassification {
  OnLeft,     /**< Point is to left of edge */
  OnRight,    /**< Point is to right of edge */
  Beyond,     /**< Point is right of edge destination */
  Behind,     /**< Point is left of edge origin */
  Between,    /**< Point is between edge origin and destination */
  Origin,     /**< Point equals edge origin */
  Destination /**< Point equals edge destination */
};
/// Helper function for classification
MANTID_GEOMETRY_DLL PointClassification classify(const Kernel::V2D &pt, const PolygonEdge &edge);

/// Calculate the orientation type of one edge wrt to another
MANTID_GEOMETRY_DLL PolygonEdge::Orientation orientation(const PolygonEdge &focusEdge, const PolygonEdge &refEdge,
                                                         double &t);
/// Calculate the crossing point of one edge with wrt another
MANTID_GEOMETRY_DLL PolygonEdge::Orientation crossingPoint(const PolygonEdge &edgeOne, const PolygonEdge &edgeTwo,
                                                           Kernel::V2D &crossPoint);
/// Return if the edges aim at each other
MANTID_GEOMETRY_DLL bool edgeAimsAt(const PolygonEdge &a, const PolygonEdge &b, PointClassification aclass,
                                    PolygonEdge::Orientation crossType);

} // namespace Geometry
} // namespace Mantid
