#ifndef MANTID_GEOMETRY_POLYGONEDGE_H_
#define MANTID_GEOMETRY_POLYGONEDGE_H_
    
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V2D.h"

namespace Mantid
{
namespace Geometry
{
  /** PolygonEdge 
    Defines a directed edge between two points on a polygon

    @author Martyn Gigg
    @date 2011-07-08

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport PolygonEdge 
  {
  public:
    /// Defines the orientation with respect to another edge
    enum Orientation {
      Collinear,      /**< Edges lie on the same line */
      Parallel,       /**< Edges point in the same direction */
      Skew,           /**< Edges are at an angle to each other */
      SkewCross,     /**< Edges are at an angle and intersect */
      SkewNoCross   /**< Edges are at an angle and do not intersect */
    };

    /// Constructor with starting and ending points
    PolygonEdge(const Kernel::V2D & start, const Kernel::V2D & end);
    /// Access the start point
    inline const Kernel::V2D & start() const { return m_start; }
    /// Access the end point
    inline const Kernel::V2D & end() const { return m_end; }
    /// Return the direction
    inline Kernel::V2D direction() const { return m_end - m_start; } 
    /// Create a point a given fraction along this edge
    Kernel::V2D point(const double fraction) const;

  private:
    /// Default constructor
    PolygonEdge();

    /// Origin point
    const Kernel::V2D m_start;
    /// Destination point
    const Kernel::V2D m_end;
  };

  /// Enumeration for point type w.r.t an edge
  enum PointClassification {
    OnLeft,         /**< Point is to left of edge */
    OnRight,        /**< Point is to right of edge */
    Beyond,       /**< Point is right of edge destination */
    Behind,       /**< Point is left of edge origin */
    Between,      /**< Point is between edge origin and destination */
    Origin,       /**< Point equals edge origin */
    Destination   /**< Point equals edge destination */
  };
  /// Helper function for classification
  MANTID_GEOMETRY_DLL PointClassification classify(const Kernel::V2D & pt, const PolygonEdge & edge);

  /// Calculate the orientation type of one edge wrt to another
  MANTID_GEOMETRY_DLL PolygonEdge::Orientation orientation(const PolygonEdge & focusEdge, const PolygonEdge & refEdge, double & t);
  /// Calculate the crossing point of one edge with wrt another
  MANTID_GEOMETRY_DLL PolygonEdge::Orientation crossingPoint(const PolygonEdge & edgeOne, const PolygonEdge & edgeTwo, Kernel::V2D & crossPoint);
  /// Return if the edges aim at each other
  MANTID_GEOMETRY_DLL bool edgeAimsAt(const PolygonEdge &a, const PolygonEdge &b, PointClassification pclass, PolygonEdge::Orientation crossType);

} // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_POLYGONEDGE_H_ */
