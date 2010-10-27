#ifndef MANTID_GEOMETRY_TRACK_H_
#define MANTID_GEOMETRY_TRACK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid
{
  //----------------------------------------------------------------------
  // Forward Declaration
  //----------------------------------------------------------------------
  namespace Kernel
  {
    class Logger;
  }

  namespace Geometry
  {

    /*!
    \struct Link
    \author S. Ansell
    \author M. Gigg, Tessella plc
    \brief For a leg of a track

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    struct DLLExport Link
    {
      /**
       * Default constructor
       */
      inline Link() : entryPoint(),exitPoint(),distFromStart(), 
        distInsideObject(), componentID(NULL)
      {}

      /**
      * Constuctor
      * @param entry V3D point to start
      * @param exit V3D point to end track
      * @param totalDistance Total distance from start of track
      * @param compID An optional component identifier for the physical object hit. (Default=NULL)
      */
      inline Link(const V3D& entry,const V3D& exit, const double totalDistance, const ComponentID compID = NULL) :
        entryPoint(entry),exitPoint(exit),distFromStart(totalDistance), distInsideObject(entryPoint.distance(exitPoint)), 
        componentID(compID)
      {}
      /// Less than operator
      inline bool operator<(const Link& other) const { return distFromStart < other.distFromStart; }
      /// Less than operator
      inline bool operator<(const double& other) const { return distFromStart < other; }

      /** @name Attributes. */
      //@{
      V3D entryPoint;             ///< Entry point
      V3D exitPoint;              ///< Exit point
      double distFromStart;       ///< Total distance from track beginning
      double distInsideObject;    ///< Total distance covered inside object
      ComponentID componentID;    ///< ComponentID of the intersected component
      //@}
    
    };

    /**
    * Stores a point of intersection along a track. The component intersected
    * is linked using its ComponentID.
    *
    * Ordering for IntersectionPoint is special since we need that when dist is close 
    * that the +/- flag is taken into
    * account.
    */
    struct IntersectionPoint
    {
      /**
      * Constuctor
      * @param flag Indicates the direction of travel of the track with respect 
      * to the object: +1 is entering, -1 is leaving.
      * @param end The end point for this partial segment
      * @param distFromStartOfTrack Total distance from start of track
      * @param compID An optional unique ID marking the component intersected. (Default=NULL)
      */
      inline IntersectionPoint(const int flag, const Geometry::V3D& end,
                               const double distFromStartOfTrack, const ComponentID compID = NULL) :
        directionFlag(flag),endPoint(end),distFromStart(distFromStartOfTrack), componentID(compID)
      {}

      /**
      * A IntersectionPoint is less-than another if either
      * (a) the difference in distances is greater than the tolerance and this distance is less than the other or
      * (b) the distance is less than the other and this point is defined as an exit point
      * 
      * @param other IntersectionPoint object to compare
      * @return True if the object is considered less than, otherwise false.
      */
      inline bool operator<(const IntersectionPoint& other) const
      {
        const double diff = fabs(distFromStart - other.distFromStart);
        return (diff > Tolerance) ? distFromStart < other.distFromStart : directionFlag < other.directionFlag;
      }

      /** @name Attributes. */
      //@{
      int directionFlag;         ///< Directional flag
      V3D endPoint;              ///< Point
      double distFromStart;      ///< Total distance from track begin
      ComponentID componentID;   ///< Unique component ID
      //@}
    };

    /**
    * Defines a track as a start point and a direction. Intersections are
    * stored as ordered lists of links from the start point to the exit point.
    *
    * @author S. Ansell
    */
    class DLLExport Track
    {
    public:
      typedef std::vector<Link> LType;       ///< Type for the Link storage
      typedef std::vector<IntersectionPoint> PType;   ///< Type for the partial

    public:
      /// Constructor
      Track(const V3D& startPt, const V3D& unitVector);
      /// Copy constructor
      Track(const Track&);
      /// Assignment operator
      Track& operator=(const Track&);
      /// Destructor
      ~Track();
      /// Adds a point of intersection to the track
      void addPoint(const int directionFlag, const V3D& endPoint, const ComponentID compID = NULL);
      /// Adds a link to the track
      int addLink(const V3D& firstPoint,const V3D& secondPoint, 
        const double distanceAlongTrack, const ComponentID compID = NULL);
      /// Remove touching Links that have identical components
      void removeCojoins();
      /// Construct links between added points
      void buildLink();

      /// Reset the track
      void reset(const Geometry::V3D& startPoint,const Geometry::V3D& direction);
      /// Returns the starting point
      const V3D& startPoint() const { return m_startPoint; }
      /// Returns the direction as a unit vector
      const V3D& direction() const { return m_unitVector; }
      /// Returns an interator to the start of the set of links
      LType::const_iterator begin() const { return m_links.begin(); }
      /// Returns an interator to one-past-the-end of the set of links
      LType::const_iterator end() const { return m_links.end(); }       
      /// Returns the number of links
      int count() const { return static_cast<int>(m_links.size()); }     
      /// Is the link complete? 
      int nonComplete() const;

    private:
      V3D m_startPoint;   ///< Start Point
      V3D m_unitVector;  ///< unit vector to direction
      LType m_links;          ///< Track units
      PType m_surfPoints;    ///< Intersection points
    };

  }  // NAMESPACE Geometry
}  // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_TRACK_H_*/
