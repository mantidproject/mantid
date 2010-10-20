#include <fstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#include "MantidKernel/Exception.h"
#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Surfaces/Surface.h"

namespace Mantid
{
  namespace Geometry
  {

    /**
    * Constructor
    * @param startPoint Initial point
    * @param direction Directional vector. It must be unit vector.
    * @param initObj iniital object identifier
    */ 
    Track::Track(const Geometry::V3D& startPoint, const Geometry::V3D& direction,const int initObj) : 
    iPt(startPoint),uVec(direction),iObj(initObj)
    {}

    /**
     * Copy Constructor
     * @param other Track to initialise this copy with.
     */ 
    Track::Track(const Track& other) : iPt(other.iPt),uVec(other.uVec),iObj(other.iObj),
      Link(other.Link),surfPoints(other.surfPoints)
    {}

    /**
    * Assignment operator
    * @param other The track to copy from
    * @return *this
    */ 
    Track& Track::operator=(const Track& other)
    {
      if (this != &other)
      {
        iPt = other.iPt;
        uVec = other.uVec;
        iObj = other.iObj;
        Link = other.Link;
        surfPoints = other.surfPoints;
      }
      return *this;
    }

    /**
     * Destructor
     */
    Track::~Track()
    {}

    /**
     * Resets the track starting point and direction
     * @param startPoint The new starting point
     * @param direction The new direction
     */
    void Track::setFirst(const Geometry::V3D& startPoint,
      const Geometry::V3D& direction)
    {
      iPt = startPoint;
      uVec = direction;
    }

    /**
     * Determines if the track is complete
     * @retval 0 :: Complete from Init to end without gaps
     * @retval +ve :: Index number of incomplete segment +1
     */
    int Track::nonComplete() const
    {
      const double TrackTolerance(1e-6);
      if (Link.size()<2)
        return 0;

      LType::const_iterator ac=Link.begin();
      if (iPt.distance(ac->entryPoint)>TrackTolerance)
        return 1;
      LType::const_iterator bc=ac;
      bc++;

      while(bc!=Link.end())
      {
        if ((ac->exitPoint).distance(bc->entryPoint)>TrackTolerance)
          return distance(Link.begin(),bc)+1;
        ac++;
        bc++;
      }
      // success
      return 0;
    }

    /**
    * Remove touching TUnits that have identical
    * components
    */
    void Track::removeCoJoins()
    {
      // No work to do:
      if (Link.empty())
        return; 
      // ac == previous : bc = next node.
      LType::iterator ac = Link.begin();
      LType::iterator bc = Link.begin();
      bc++;
      while(bc != Link.end())
      {
        if (ac->ObjID == bc->ObjID)
        {
          ac->exitPoint = bc->exitPoint;
          ac->distFromStart = ac->entryPoint.distance(ac->exitPoint);
          ac->distInsideObject = bc->distInsideObject;
          Link.erase(bc);
          bc = ac;
          bc++;
        }
        else
        {
          ac++;
          bc++;
        }
      }
      return;
    }

    /**
    * Objective is to merge in partial information
    * about the beginning and end of the tracks.
    * We do not need to keep surfPoints in order
    * because that will be done when they are converted into
    * TUnits.  
    * @param ID Id number of the object
    * @param Direct direction of travel
    * @param Pt Point to go
    */
    void Track::addPoint(const int ID,const int Direct,
      const Geometry::V3D& Pt) 
    {
      surfPoints.push_back(TPartial(ID,Direct,Pt,Pt.distance(iPt)));
      return;
    }

    /**
    * This adds a whole segment to the track : This currently assumes that links are added in order
    * @param ID :: Id number of the object
    * @param startPoint :: first Point
    * @param endPoint :: second Point
    * @param distAlongTrack :: Distance along track
    * @retval Index point 
    */
    int Track::addTUnit(const int ID,const Geometry::V3D& startPoint,
      const Geometry::V3D& endPoint,const double distAlongTrack)

    {
      // Process First Point
      TUnit newTUnit(startPoint,endPoint,distAlongTrack,ID);
      if (Link.empty())
      {
        Link.push_back(newTUnit);
        return 0;
      }
      std::vector<TUnit>::iterator xV = lower_bound(Link.begin(),Link.end(),newTUnit);

      //must extract the distance before you insert otherwise the iterators are incompatible
      int index = distance(Link.begin(),xV);

      Link.insert(xV,newTUnit);

      return index;
    }

     /**
      * Builds a set of linking track components.
      * This version deals with touching surfaces 
      */
    void Track::buildLink()
    {
      if (surfPoints.empty())
        return;

      // First sort surfPoints
      sort(surfPoints.begin(),surfPoints.end());
      PType::const_iterator ac=surfPoints.begin();
      PType::const_iterator bc=ac;
      bc++;
      Geometry::V3D workPt=iPt;            // last good point
      // First point is not necessarily in an object
      // Process first point:
      while(ac!=surfPoints.end() && ac->direction != 1)    // stepping from an object.
      {
        if (ac->direction==-1)
        {
          addTUnit(ac->ObjID,iPt,ac->endPoint,ac->distFromStart);  // from the void
          workPt = ac->endPoint;
        }
        ac++;
        if (bc!=surfPoints.end())
          bc++;
      } 

      //have we now passed over all of the potential intersections without actually hitting the object
      if (ac==surfPoints.end())
      {
        //yes
        surfPoints.clear();
        return;
      }

      workPt = ac->endPoint;      

      while(bc!=surfPoints.end())      // Since bc > ac
      {
        if (ac->direction==1 && bc->direction==-1)
        {
          // Touching surface / identical surface
          if (fabs(ac->distFromStart - bc->distFromStart)>Tolerance)
          {
            // track leave ac into bc.
            addTUnit(ac->ObjID,ac->endPoint,bc->endPoint,bc->distFromStart);
          }
          // Points with intermediate void
          else
          {
            addTUnit(ac->ObjID,workPt,ac->endPoint,ac->distFromStart);
          }
          workPt=bc->endPoint;

          // ADDING to ac twice: since processing pairs
          ac++;
          ac++;
          bc++;    // can I do this past the end ? 
          if (bc!=surfPoints.end())
            bc++;
        }
        else         // Test for glacing point / or void edges
        {          // These all can be skipped
          ac++;
          bc++;
        }
      }	

      surfPoints.clear();        // While vector 
      return;
    }

  } // NAMESPACE Geometry

}  // NAMESPACE Mantid
