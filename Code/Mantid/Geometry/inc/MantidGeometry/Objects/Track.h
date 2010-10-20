#ifndef MANTID_GEOMETRY_TRACK_H_
#define MANTID_GEOMETRY_TRACK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Tolerance.h"

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
    \struct TUnit
    \version 1.0
    \author S. Ansell
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
    struct DLLExport TUnit
    {
      /**
      * Constuctor
      * @param entry V3D point to start
      * @param exit V3D point to end track
      * @param totalDistance Total distance from start of track
      * @param ID ID number
      */
      inline TUnit(const Geometry::V3D& entry,const Geometry::V3D& exit, const double totalDistance,const int ID) :
      entryPoint(entry),exitPoint(exit),distFromStart(totalDistance), distInsideObject(entryPoint.distance(exitPoint)),ObjID(ID)
      {}
      /// Less than operator
      inline bool operator<(const TUnit& other) const { return distFromStart < other.distFromStart; }
      /// Less than operator
      inline bool operator<(const double& other) const { return distFromStart < other; }

      /** @name Attributes. */
      //@{
      V3D entryPoint;           ///< Entry point
      V3D exitPoint;           ///< Exit point
      double distFromStart;         ///< Total distance from track beginning
      double distInsideObject;       ///< Total distance covered inside object
      int ObjID;           ///< ObjectID
      //@}
    };

    /**
    * Stores a point of intersection along a track. The object intersected is linked using its
    * ID.
    *
    * Ordering for TPartial is special since we need
    * that when dist is close that the +/- flag is taken into
    * account.
    */
    struct TPartial
    {
      /**
      * Constuctor
      * @param ID Object ID number
      * @param flag Indicates the direction of travel of the track with respect to the object: +1 is entering, -1 is leaving.
      * @param end The end point for this partial segment
      * @param distFromStartOfTrack Total distance from start of track
      */
      inline TPartial(const int ID,const int directionFlag, const Geometry::V3D& end,const double distFromStartOfTrack) :
      ObjID(ID),direction(directionFlag),endPoint(end),distFromStart(distFromStartOfTrack)
      {}

      /**
      * A TPartial is less-than another if either
      * (a) the difference in distances is greater than the tolerance and this distance is less than the other or
      * (b) the distance is less than the other and this point is defined as an exit point
      * 
      * @param other TPartial object to compare
      * @return True if the object is considered less than, otherwise false.
      */
      inline bool operator<(const TPartial& other) const
      {
        const double diff = fabs(distFromStart - other.distFromStart);
        return (diff > Tolerance) ? distFromStart < other.distFromStart : direction < other.direction;
      }

      /** @name Attributes. */
      //@{
      int ObjID;           ///< ObjectID
      int direction;            ///< Directional flag
      V3D endPoint;             ///< Point
      double distFromStart;         ///< Total distance from track begin
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
      typedef std::vector<TUnit> LType;       ///< Type for the Link storage
      typedef std::vector<TPartial> PType;    ///< Type for the partial

    public:
      /// Constructor
      Track(const Geometry::V3D& StartPt,const Geometry::V3D& UV,const int initObj=0);
      /// Copy constructor
      Track(const Track&);
      /// Assignment operator
      Track& operator=(const Track&);
      /// Destructor
      ~Track();
      /// Adds a point of intersection to the track
      void addPoint(const int ID,const int Direct,const Geometry::V3D& Pt);
      /// Adds a link to the track
      int addTUnit(const int ID,const Geometry::V3D& Apt,const Geometry::V3D& Bpt,
        const double D);
      /// Remove touching TUnits that have identical components
      void removeCoJoins();
      /// Construct links between added points
      void buildLink();

      /// Reset the track
      void setFirst(const Geometry::V3D& startPoint,const Geometry::V3D& direction);
      /// Returns the starting point
      const Geometry::V3D& getInit() const { return iPt; }
      /// Returns the direction
      const Geometry::V3D& getUVec() const { return uVec; }
      /// Returns an interator to the start of the set of links
      LType::const_iterator begin() const { return Link.begin(); }
      /// Returns an interator to one-past-the-end of the set of links
      LType::const_iterator end() const { return Link.end(); }       
      /// Returns the number of links
      int count() const { return static_cast<int>(Link.size()); }     
      /// Is the link complete? 
      int nonComplete() const;

    private:
      Geometry::V3D iPt;   ///< Start Point
      Geometry::V3D uVec;  ///< unit vector to direction
      int iObj;            ///< Initial object
      LType Link;          ///< Track units
      PType surfPoints;    ///< Intersection points

    };

  }  // NAMESPACE Geometry
}  // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_TRACK_H_*/
