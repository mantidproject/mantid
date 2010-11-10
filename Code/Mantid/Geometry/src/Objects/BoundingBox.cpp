//---------------------------------------------------------
// Includes
//---------------------------------------------------------
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Track.h"

namespace Mantid
{
  namespace Geometry
  {
    //---------------------------------------------------------
    // Public member functions
    //---------------------------------------------------------
    /**
    * Query whether the given point is inside the bounding box within a tolerance defined by Mantid::Geometry::Tolerance.
    * @param point The point to query
    * @returns True if the point is within the bounding box, false otherwise
    */
    bool BoundingBox::isPointInside(const V3D & point) const
    {
      if(point.X() <= xMax() + Tolerance && point.X() >= xMin() - Tolerance &&
         point.Y() <= yMax() + Tolerance && point.Y() >= yMin() - Tolerance && 
        point.Z() <= zMax() + Tolerance && point.Z() >= zMin() - Tolerance)
      {
        return true;
      }
      else
      {
        return false;
      }
    }

    /** 
    * Does a defined track intersect the bounding box
    * @param track A test track It is assumed that this is outside the bounding box.
    * @returns True if the track intersects this bounding box, false otherwise.
    */
    bool BoundingBox::doesLineIntersect(const Track & track) const
    {
      return this->doesLineIntersect(track.startPoint(), track.direction());
    }

    /** 
    * Does a line intersect the bounding box
    * @param startPoint The starting point for the line. It is assumed that this is outside the bounding box.
    * @param lineDir The direction of the line
    * @returns True if the line intersects this bounding box, false otherwise.
    */
    bool BoundingBox::doesLineIntersect(const V3D & startPoint, const V3D & lineDir) const
    {
      // Method - Loop through planes looking for ones that are visible and check intercept
      // Assume that orig is outside of BoundingBox.
      const double tol = Mantid::Geometry::Tolerance;
      double lambda(0.0);
      if (startPoint.X() > xMax())
      {
        if (lineDir.X() < -tol)
        {
          lambda = (xMax() - startPoint.X()) / lineDir.X();
          if (yMin() < startPoint.Y() + lambda * lineDir.Y() && yMax() > startPoint.Y() + lambda * lineDir.Y())
            if (zMin() < startPoint.Z() + lambda * lineDir.Z() && zMax() > startPoint.Z() + lambda * lineDir.Z())
              return true;
        }
      }
      if (startPoint.X() < xMin())
      {
        if (lineDir.X() > tol)
        {
          lambda = (xMin() - startPoint.X()) / lineDir.X();
          if (yMin() < startPoint.Y() + lambda * lineDir.Y() && yMax() > startPoint.Y() + lambda * lineDir.Y())
            if (zMin() < startPoint.Z() + lambda * lineDir.Z() && zMax() > startPoint.Z() + lambda * lineDir.Z())
              return true;
        }
      }
      if (startPoint.Y() > yMax())
      {
        if (lineDir.Y() < -tol)
        {
          lambda = (yMax() - startPoint.Y()) / lineDir.Y();
          if (xMin() < startPoint.X() + lambda * lineDir.X() && xMax() > startPoint.X() + lambda * lineDir.X())
            if (zMin() < startPoint.Z() + lambda * lineDir.Z() && zMax() > startPoint.Z() + lambda * lineDir.Z())
              return true;
        }
      }
      if (startPoint.Y() < yMin())
      {
        if (lineDir.Y() > tol)
        {
          lambda = (yMin() - startPoint.Y()) / lineDir.Y();
          if (xMin() < startPoint.X() + lambda * lineDir.X() && xMax() > startPoint.X() + lambda * lineDir.X())
            if (zMin() < startPoint.Z() + lambda * lineDir.Z() && zMax() > startPoint.Z() + lambda * lineDir.Z())
              return true;
        }
      }
      if (startPoint.Z() > zMax())
      {
        if (lineDir.Z() < -tol)
        {
          lambda = (zMax() - startPoint.Z()) / lineDir.Z();
          if (yMin() < startPoint.Y() + lambda * lineDir.Y() && yMax() > startPoint.Y() + lambda * lineDir.Y())
            if (xMin() < startPoint.X() + lambda * lineDir.X() && xMax() > startPoint.X() + lambda * lineDir.X())
              return true;
        }
      }
      if (startPoint.Z() < zMin())
      {
        if (lineDir.Z() > tol)
        {
          lambda = (zMin() - startPoint.Z()) / lineDir.Z();
          if (yMin() < startPoint.Y() + lambda * lineDir.Y() && yMax() > startPoint.Y() + lambda * lineDir.Y())
            if (xMin() < startPoint.X() + lambda * lineDir.X() && xMax() > startPoint.X() + lambda * lineDir.X())
              return true;
        }
      }
      return false;
    }

    /**
     * Find maximum angular half width of the bounding box from the observer, that is
     * the greatest angle between the centre point and any corner point
     * @param observer Viewing point
     * @returns The value of the angular half-width
    */
    double BoundingBox::angularWidth(const Geometry::V3D& observer) const
    {
      Geometry::V3D centre = centrePoint() - observer;
      std::vector<Geometry::V3D> pts(8);
      pts[0] = Geometry::V3D(xMin(), yMin(), zMin()) - observer;
      pts[1] = Geometry::V3D(xMin(), yMin(), zMax()) - observer;
      pts[2] = Geometry::V3D(xMin(), yMax(), zMin()) - observer;
      pts[3] = Geometry::V3D(xMin(), yMax(), zMax()) - observer;
      pts[4] = Geometry::V3D(xMax(), yMin(), zMin()) - observer;
      pts[5] = Geometry::V3D(xMax(), yMin(), zMax()) - observer;
      pts[6] = Geometry::V3D(xMin(), yMax(), zMin()) - observer;
      pts[7] = Geometry::V3D(xMin(), yMax(), zMax()) - observer;

      std::vector<Geometry::V3D>::const_iterator ip;
      double centre_norm_inv = 1.0 / centre.norm();
      double thetaMax(-1.0);
      for (ip = pts.begin(); ip != pts.end(); ip++)
      {
        double theta = acos(ip->scalar_prod(centre)*centre_norm_inv / ip->norm());
        if (theta > thetaMax)
          thetaMax = theta;
      }
      return thetaMax;
    }

    /**
     * Enlarges this bounding box so that it encompasses that given.
     * @param other The bounding box that should be encompassed
     */
    void BoundingBox::grow(const BoundingBox & other)
    {
      m_null = false;
      // If the current box is empty then we definitely need to grow
      if( minPoint() == V3D() && maxPoint() == V3D() )
      {
        m_minPoint = other.minPoint();
        m_maxPoint = other.maxPoint();
        return;
      }

      // Simply checks if an of the points in the given box are outside this one and 
      // changes the coordinate appropriately
      V3D otherPoint = other.minPoint();
      for( size_t i = 0; i < 3; ++i )
      {
        if( otherPoint[i] < m_minPoint[i] )
        {
          m_minPoint[i] = otherPoint[i];
        }
      }
      otherPoint = other.maxPoint();
      for( size_t i = 0; i < 3; ++i )
      {
        if( otherPoint[i] > m_maxPoint[i] )
        {
           m_maxPoint[i] = otherPoint[i];
        }
      }

    }

    //--------------------------------------------------------------------------
    // Namespace functions
    //--------------------------------------------------------------------------

    /**
     * Print out the bounding box values to a stream.
     * @param os The output stream
     * @param box A reference to the bounding box to print out.
     */
    std::ostream& operator<<(std::ostream& os, const BoundingBox& box)
    {
      os << "X from " <<  box.xMin() << " to " <<  box.xMax()
	 << "; Y from " <<  box.yMin() << " to " <<  box.yMax()
	 << "; Z from " <<  box.zMin() << " to " <<  box.zMax();
      return os;
    }

  }
}
