#ifndef MANTID_GEOMETRY_TRACK_H_
#define MANTID_GEOMETRY_TRACK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Tolerance.h"
#include <list>

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

    /**
    \struct Link
    \author S. Ansell
    \author M. Gigg, Tessella plc
    \brief For a leg of a track

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    struct MANTID_GEOMETRY_DLL Link
    {
      /**
      * Constuctor
      * @param entry :: Kernel::V3D point to start
      * @param exit :: Kernel::V3D point to end track
      * @param totalDistance :: Total distance from start of track
      * @param obj :: A reference to the object that was intersected
      * @param compID :: An optional component identifier for the physical object hit. (Default=NULL)
      */
      inline Link(const Kernel::V3D& entry,const Kernel::V3D& exit, const double totalDistance,
                  const Object & obj, const ComponentID compID = NULL) :
        entryPoint(entry),exitPoint(exit),distFromStart(totalDistance), distInsideObject(entryPoint.distance(exitPoint)),
        object(&obj), componentID(compID)
      {}
      /// Less than operator
      inline bool operator<(const Link& other) const { return distFromStart < other.distFromStart; }
      /// Less than operator
      inline bool operator<(const double& other) const { return distFromStart < other; }

      /** @name Attributes. */
      //@{
      Kernel::V3D entryPoint;     ///< Entry point
      Kernel::V3D exitPoint;      ///< Exit point
      double distFromStart;       ///< Total distance from track beginning
      double distInsideObject;    ///< Total distance covered inside object
      const Object * object;      ///< The object that was intersected
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
      * @param flag :: Indicates the direction of travel of the track with respect
      * to the object: +1 is entering, -1 is leaving.
      * @param end :: The end point for this partial segment
      * @param distFromStartOfTrack :: Total distance from start of track
      * @param compID :: An optional unique ID marking the component intersected. (Default=NULL)
      * @param obj :: A reference to the object that was intersected
      */
      inline IntersectionPoint(const int flag, const Kernel::V3D& end,
                               const double distFromStartOfTrack, const Object & obj,
                               const ComponentID compID = NULL) :
        directionFlag(flag),endPoint(end),distFromStart(distFromStartOfTrack),
        object(&obj), componentID(compID)
      {}

      /**
      * A IntersectionPoint is less-than another if either
      * (a) the difference in distances is greater than the tolerance and this distance is less than the other or
      * (b) the distance is less than the other and this point is defined as an exit point
      *
      * @param other :: IntersectionPoint object to compare
      * @return True if the object is considered less than, otherwise false.
      */
      inline bool operator<(const IntersectionPoint& other) const
      {
        const double diff = fabs(distFromStart - other.distFromStart);
        return (diff > Kernel::Tolerance) ? distFromStart < other.distFromStart : directionFlag < other.directionFlag;
      }

      /** @name Attributes. */
      //@{
      int directionFlag;         ///< Directional flag
      Kernel::V3D endPoint;              ///< Point
      double distFromStart;      ///< Total distance from track begin
      const Object * object;      ///< The object that was intersected
      ComponentID componentID;   ///< Unique component ID
      //@}
    };

    /**
    * Defines a track as a start point and a direction. Intersections are
    * stored as ordered lists of links from the start point to the exit point.
    *
    * @author S. Ansell
    */
    class MANTID_GEOMETRY_DLL Track
    {
    public:
      typedef std::list<Link> LType;       ///< Type for the Link storage
      typedef std::list<IntersectionPoint> PType;   ///< Type for the partial

    public:
      /// Default constructor
      Track();
      /// Constructor
      Track(const Kernel::V3D& startPt, const Kernel::V3D& unitVector);
      /// Copy constructor
      Track(const Track&);
      /// Assignment operator
      Track& operator=(const Track&);
      /// Destructor
      ~Track();
      /// Adds a point of intersection to the track
      void addPoint(const int directionFlag, const Kernel::V3D& endPoint,
                    const Object & obj, const ComponentID compID = NULL);
      /// Adds a link to the track
      int addLink(const Kernel::V3D& firstPoint,const Kernel::V3D& secondPoint,
                  const double distanceAlongTrack, const Object & obj,
                  const ComponentID compID = NULL);
      /// Remove touching Links that have identical components
      void removeCojoins();
      /// Construct links between added points
      void buildLink();

      /// Set a starting point and direction
      void reset(const Kernel::V3D& startPoint,const Kernel::V3D& direction);
      /// Clear the current set of intersection results
      void clearIntersectionResults();
      /// Returns the starting point
      const Kernel::V3D& startPoint() const { return m_startPoint; }
      /// Returns the direction as a unit vector
      const Kernel::V3D& direction() const { return m_unitVector; }
      /// Returns an interator to the start of the set of links
      LType::const_iterator begin() const { return m_links.begin(); }
      /// Returns an interator to one-past-the-end of the set of links
      LType::const_iterator end() const { return m_links.end(); }
      /// Returns the number of links
      int count() const { return static_cast<int>(m_links.size()); }
      /// Is the link complete?
      int nonComplete() const;

    private:
      Kernel::V3D m_startPoint;   ///< Start Point
      Kernel::V3D m_unitVector;  ///< unit vector to direction
      LType m_links;          ///< Track units
      PType m_surfPoints;    ///< Intersection points
    };

  }  // NAMESPACE Geometry
}  // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_TRACK_H_*/
