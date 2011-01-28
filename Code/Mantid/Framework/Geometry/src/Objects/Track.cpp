#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Surfaces/Surface.h"

#include <cmath>
#include <algorithm>
#include <iostream>

namespace Mantid
{
  namespace Geometry
  {
    /**
     * Default constructor
     */
    Track::Track() : m_startPoint(), m_unitVector()
    {
    }

    /**
    * Constructor
    * @param startPoint :: Initial point
    * @param direction :: Directional vector. It must be unit vector.
    */ 
    Track::Track(const V3D& startPoint, const V3D& direction) : 
    m_startPoint(startPoint),m_unitVector(direction)
    {}

    /**
     * Copy Constructor
     * @param other :: Track to initialise this copy with.
     */ 
    Track::Track(const Track& other) : m_startPoint(other.m_startPoint),m_unitVector(other.m_unitVector),
      m_links(other.m_links),m_surfPoints(other.m_surfPoints)
    {}

    /**
    * Assignment operator
    * @param other :: The track to copy from
    * @return *this
    */ 
    Track& Track::operator=(const Track& other)
    {
      if (this != &other)
      {
        m_startPoint = other.m_startPoint;
        m_unitVector = other.m_unitVector;
        m_links = other.m_links;
        m_surfPoints = other.m_surfPoints;
      }
      return *this;
    }

    /**
     * Destructor
     */
    Track::~Track()
    {}

    /**
     * Resets the track starting point and direction. 
     * @param startPoint :: The new starting point
     * @param direction :: The new direction
     */
    void Track::reset(const V3D& startPoint, const V3D& direction)
    {
      m_startPoint = startPoint;
      m_unitVector = direction;
    }

    /**
     * Clear the current set of intersection results
     */
    void Track::clearIntersectionResults()
    {
      m_links.clear();
      m_surfPoints.clear();
    }

    /**
     * Determines if the track is complete
     * @retval 0 :: Complete from Init to end without gaps
     * @retval +ve :: Index number of incomplete segment +1
     */
    int Track::nonComplete() const
    {
      if (m_links.size() < 2)
      {
        return 0;
      }
      LType::const_iterator ac = m_links.begin();
      if (m_startPoint.distance(ac->entryPoint) > Tolerance)
      {
        return 1;
      }
      LType::const_iterator bc = ac;
      bc++;

      while(bc != m_links.end())
      {
        if( (ac->exitPoint).distance(bc->entryPoint) > Tolerance)
        {
          return (distance(m_links.begin(),bc) + 1);
        }
        ac++;
        bc++;
      }
      // success
      return 0;
    }

    /**
     * Remove touching links that have identical
     * components
     */
    void Track::removeCojoins()
    {
      if( m_links.empty() )
      {
        return;
      }
      LType::iterator prevNode = m_links.begin();
      LType::iterator nextNode = m_links.begin();
      nextNode++;
      while(nextNode != m_links.end())
      {
        if(prevNode->componentID == nextNode->componentID)
        {
          prevNode->exitPoint = nextNode->exitPoint;
          prevNode->distFromStart = prevNode->entryPoint.distance(prevNode->exitPoint);
          prevNode->distInsideObject = nextNode->distInsideObject;
          m_links.erase(nextNode);
          nextNode = prevNode;
          nextNode++;
        }
        else
        {
          prevNode++;
          nextNode++;
        }
      }
      return;
    }

    /**
     * Objective is to merge in partial information about the beginning and end of the tracks.
     * The points are kept in order
     * @param directionFlag :: A flag indicating if the direction of travel is entering/leaving
     * an object. +1 is entering, -1 is leaving.
     * @param point :: Point of intersection
     * @param compID :: ID of the component that this link is about (Default=NULL)
     */
    void Track::addPoint(const int directionFlag, const V3D& point, const ComponentID compID) 
    {
      IntersectionPoint newPoint(directionFlag, point, point.distance(m_startPoint), compID);
      PType::iterator lowestPtr = std::lower_bound(m_surfPoints.begin(), m_surfPoints.end(), newPoint);
      m_surfPoints.insert(lowestPtr, newPoint);
    }

    /**
    * This adds a whole segment to the track : This currently assumes that links are added in order
    * @param startPoint :: first Point
    * @param endPoint :: second Point
    * @param distAlongTrack :: Distance along track
    * @param compID :: ID of the component that this link is about (Default=NULL)
    * @retval Index of link within the track
    */
    int Track::addLink(const V3D& startPoint, const V3D& endPoint,
      const double distAlongTrack, const ComponentID compID)
    {
      // Process First Point
      Link newLink(startPoint,endPoint,distAlongTrack,compID);
      int index(0);
      if( m_links.empty() )
      {
        m_links.push_back(newLink);
        index = 0;
      }
      else
      {
        LType::iterator linkPtr = std::lower_bound(m_links.begin(),m_links.end(),newLink);
        //must extract the distance before you insert otherwise the iterators are incompatible
        index = std::distance(m_links.begin(), linkPtr);
        m_links.insert(linkPtr,newLink);
      }
      return index;
    }

     /**
      * Builds a set of linking track components.
      * This version deals with touching surfaces 
      */
    void Track::buildLink()
    {
      if (m_surfPoints.empty())
      {
        return;
      }

      // The surface points were added in order when they were built so no sorting is required here.
      PType::const_iterator ac = m_surfPoints.begin();
      PType::const_iterator bc = ac;
      bc++;
      V3D workPt = m_startPoint;            // last good point
      // First point is not necessarily in an object
      // Process first point:
      while(ac!=m_surfPoints.end() && ac->directionFlag != 1)    // stepping from an object.
      {
        if (ac->directionFlag==-1)
        {
          addLink(m_startPoint,ac->endPoint,ac->distFromStart,ac->componentID);  // from the void
          workPt = ac->endPoint;
        }
        ac++;
        if (bc!=m_surfPoints.end())
        {
          bc++;
        }
      } 

      //have we now passed over all of the potential intersections without actually hitting the object
      if (ac == m_surfPoints.end())
      {
        //yes
        m_surfPoints.clear();
        return;
      }

      workPt = ac->endPoint;      
      while(bc != m_surfPoints.end())      // Since bc > ac
      {
        if (ac->directionFlag==1 && bc->directionFlag==-1)
        {
          // Touching surface / identical surface
          if (fabs(ac->distFromStart - bc->distFromStart)>Tolerance)
          {
            // track leave ac into bc.
            addLink(ac->endPoint,bc->endPoint,bc->distFromStart,ac->componentID);
          }
          // Points with intermediate void
          else
          {
            addLink(workPt,ac->endPoint,ac->distFromStart,ac->componentID);
          }
          workPt = bc->endPoint;

          // ADDING to ac twice: since processing pairs
          ac++;
          ac++;
          bc++;    // can I do this past the end ? 
          if (bc!=m_surfPoints.end())
          {
            bc++;
          }
        }
        else         // Test for glacing point / or void edges
        {          // These all can be skipped
          ac++;
          bc++;
        }
      }	

      m_surfPoints.clear();        // While vector 
      return;
    }

  } // NAMESPACE Geometry

}  // NAMESPACE Mantid
