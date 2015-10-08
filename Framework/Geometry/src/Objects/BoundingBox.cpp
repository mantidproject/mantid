//---------------------------------------------------------
// Includes
//---------------------------------------------------------
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Track.h"
#include <cfloat>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;

//---------------------------------------------------------
// Public member functions
//---------------------------------------------------------
/**
* Query whether the given point is inside the bounding box within a tolerance
* defined by Mantid::Geometry::Tolerance.
* @param point :: The point to query
* @returns True if the point is within the bounding box, false otherwise
*/
bool BoundingBox::isPointInside(const V3D &point) const {
  if (!this->isAxisAligned()) {
    throw(Kernel::Exception::NotImplementedError(
        "this function has not been modified properly"));
  }

  if (point.X() <= xMax() + Kernel::Tolerance &&
      point.X() >= xMin() - Kernel::Tolerance &&
      point.Y() <= yMax() + Kernel::Tolerance &&
      point.Y() >= yMin() - Kernel::Tolerance &&
      point.Z() <= zMax() + Kernel::Tolerance &&
      point.Z() >= zMin() - Kernel::Tolerance) {
    return true;
  } else {
    return false;
  }
}

/**
* Does a defined track intersect the bounding box
* @param track :: A test track It is assumed that this is outside the bounding
* box.
* @returns True if the track intersects this bounding box, false otherwise.
*/
bool BoundingBox::doesLineIntersect(const Track &track) const {
  return this->doesLineIntersect(track.startPoint(), track.direction());
}

/**
* Does a line intersect the bounding box
* @param startPoint :: The starting point for the line. It is assumed that this
* is outside the bounding box.
* @param lineDir :: The direction of the line
* @returns True if the line intersects this bounding box, false otherwise.
*/
bool BoundingBox::doesLineIntersect(const V3D &startPoint,
                                    const V3D &lineDir) const {
  if (!this->isAxisAligned()) {
    throw(Kernel::Exception::NotImplementedError(
        "this function has not been modified properly"));
  }
  // Method - Loop through planes looking for ones that are visible and check
  // intercept
  // Assume that orig is outside of BoundingBox.
  const double tol = Mantid::Kernel::Tolerance;
  double lambda(0.0);
  if (startPoint.X() > xMax()) {
    if (lineDir.X() < -tol) {
      lambda = (xMax() - startPoint.X()) / lineDir.X();
      if (yMin() < startPoint.Y() + lambda * lineDir.Y() &&
          yMax() > startPoint.Y() + lambda * lineDir.Y())
        if (zMin() < startPoint.Z() + lambda * lineDir.Z() &&
            zMax() > startPoint.Z() + lambda * lineDir.Z())
          return true;
    }
  }
  if (startPoint.X() < xMin()) {
    if (lineDir.X() > tol) {
      lambda = (xMin() - startPoint.X()) / lineDir.X();
      if (yMin() < startPoint.Y() + lambda * lineDir.Y() &&
          yMax() > startPoint.Y() + lambda * lineDir.Y())
        if (zMin() < startPoint.Z() + lambda * lineDir.Z() &&
            zMax() > startPoint.Z() + lambda * lineDir.Z())
          return true;
    }
  }
  if (startPoint.Y() > yMax()) {
    if (lineDir.Y() < -tol) {
      lambda = (yMax() - startPoint.Y()) / lineDir.Y();
      if (xMin() < startPoint.X() + lambda * lineDir.X() &&
          xMax() > startPoint.X() + lambda * lineDir.X())
        if (zMin() < startPoint.Z() + lambda * lineDir.Z() &&
            zMax() > startPoint.Z() + lambda * lineDir.Z())
          return true;
    }
  }
  if (startPoint.Y() < yMin()) {
    if (lineDir.Y() > tol) {
      lambda = (yMin() - startPoint.Y()) / lineDir.Y();
      if (xMin() < startPoint.X() + lambda * lineDir.X() &&
          xMax() > startPoint.X() + lambda * lineDir.X())
        if (zMin() < startPoint.Z() + lambda * lineDir.Z() &&
            zMax() > startPoint.Z() + lambda * lineDir.Z())
          return true;
    }
  }
  if (startPoint.Z() > zMax()) {
    if (lineDir.Z() < -tol) {
      lambda = (zMax() - startPoint.Z()) / lineDir.Z();
      if (yMin() < startPoint.Y() + lambda * lineDir.Y() &&
          yMax() > startPoint.Y() + lambda * lineDir.Y())
        if (xMin() < startPoint.X() + lambda * lineDir.X() &&
            xMax() > startPoint.X() + lambda * lineDir.X())
          return true;
    }
  }
  if (startPoint.Z() < zMin()) {
    if (lineDir.Z() > tol) {
      lambda = (zMin() - startPoint.Z()) / lineDir.Z();
      if (yMin() < startPoint.Y() + lambda * lineDir.Y() &&
          yMax() > startPoint.Y() + lambda * lineDir.Y())
        if (xMin() < startPoint.X() + lambda * lineDir.X() &&
            xMax() > startPoint.X() + lambda * lineDir.X())
          return true;
    }
  }
  return this->isPointInside(startPoint);
}

/**
 * Find maximum angular half width of the bounding box from the observer, that
 * is
 * the greatest angle between the centre point and any corner point
 * @param observer :: Viewing point
 * @returns The value of the angular half-width
*/
double BoundingBox::angularWidth(const Kernel::V3D &observer) const {
  Kernel::V3D centre = centrePoint() - observer;
  std::vector<Kernel::V3D> pts;
  this->getFullBox(pts, observer);

  std::vector<Kernel::V3D>::const_iterator ip;
  double centre_norm_inv = 1.0 / centre.norm();
  double thetaMax(-1.0);
  for (ip = pts.begin(); ip != pts.end(); ++ip) {
    double theta = acos(ip->scalar_prod(centre) * centre_norm_inv / ip->norm());
    if (theta > thetaMax)
      thetaMax = theta;
  }
  return thetaMax;
}
//
void BoundingBox::getFullBox(std::vector<Kernel::V3D> &box,
                             const Kernel::V3D &observer) const {
  box.resize(8);
  box[0] = Kernel::V3D(xMin(), yMin(), zMin()) - observer;
  box[1] = Kernel::V3D(xMax(), yMin(), zMin()) - observer;
  box[2] = Kernel::V3D(xMax(), yMax(), zMin()) - observer;
  box[3] = Kernel::V3D(xMin(), yMax(), zMin()) - observer;
  box[4] = Kernel::V3D(xMin(), yMax(), zMax()) - observer;
  box[5] = Kernel::V3D(xMin(), yMin(), zMax()) - observer;
  box[6] = Kernel::V3D(xMax(), yMin(), zMax()) - observer;
  box[7] = Kernel::V3D(xMax(), yMax(), zMax()) - observer;
}
void BoundingBox::setBoxAlignment(const Kernel::V3D &R0,
                                  const std::vector<Kernel::V3D> &orts) {
  this->coord_system.resize(4);
  coord_system[0] = R0;
  coord_system[1] = orts[0];
  coord_system[2] = orts[1];
  coord_system[3] = orts[2];
  is_axis_aligned = false;
}
void BoundingBox::nullify() {
  this->m_null = true;
  for (int i = 0; i < 3; i++) {
    this->m_minPoint[i] = FLT_MAX;
    this->m_maxPoint[i] = -FLT_MAX;
  }
}
//
void BoundingBox::realign(std::vector<Kernel::V3D> const *const pCS) {
  if (pCS) {
    this->coord_system.resize(pCS->size());
    for (unsigned int i = 0; i < pCS->size(); i++) {
      this->coord_system[i] = pCS->operator[](i);
    }
    this->is_axis_aligned = false;
    if (this->m_null)
      return;
  } else {
    if (this->isAxisAligned())
      return;
  }

  // expand the bounding box to full size and shift it to the coordinates with
  // the
  // centre cpecified;
  std::vector<V3D> BBpoints;
  this->getFullBox(BBpoints, this->coord_system[0]);

  // identify min-max vrt the new coordinate system;
  double xMin(FLT_MAX), yMin(FLT_MAX), zMin(FLT_MAX);
  double xMax(-FLT_MAX), yMax(-FLT_MAX), zMax(-FLT_MAX);
  for (unsigned int i = 0; i < 8; i++) {
    double x = coord_system[1].scalar_prod(BBpoints[i]);
    double y = coord_system[2].scalar_prod(BBpoints[i]);
    double z = coord_system[3].scalar_prod(BBpoints[i]);
    if (x < xMin)
      xMin = x;
    if (x > xMax)
      xMax = x;
    if (y < yMin)
      yMin = y;
    if (y > yMax)
      yMax = y;
    if (z < zMin)
      zMin = z;
    if (z > zMax)
      zMax = z;
  }
  this->xMin() = xMin;
  this->xMax() = xMax;
  this->yMin() = yMin;
  this->yMax() = yMax;
  this->zMin() = zMin;
  this->zMax() = zMax;
}
/**
 * Enlarges this bounding box so that it encompasses that given.
 * @param other :: The bounding box that should be encompassed
 */
void BoundingBox::grow(const BoundingBox &other) {
  m_null = false;
  // If the current box is empty then we definitely need to grow
  if (minPoint() == V3D() && maxPoint() == V3D()) {
    m_minPoint = other.minPoint();
    m_maxPoint = other.maxPoint();
    return;
  }

  // Simply checks if an of the points in the given box are outside this one and
  // changes the coordinate appropriately
  V3D otherPoint = other.minPoint();
  for (size_t i = 0; i < 3; ++i) {
    if (otherPoint[i] < m_minPoint[i]) {
      m_minPoint[i] = otherPoint[i];
    }
  }
  otherPoint = other.maxPoint();
  for (size_t i = 0; i < 3; ++i) {
    if (otherPoint[i] > m_maxPoint[i]) {
      m_maxPoint[i] = otherPoint[i];
    }
  }
}

//--------------------------------------------------------------------------
// Namespace functions
//--------------------------------------------------------------------------

/**
 * Print out the bounding box values to a stream.
 * @param os :: The output stream
 * @param box :: A reference to the bounding box to print out.
 * @return the stream representation of the bounding box
 */
std::ostream &operator<<(std::ostream &os, const BoundingBox &box) {
  os << "X from " << box.xMin() << " to " << box.xMax() << "; Y from "
     << box.yMin() << " to " << box.yMax() << "; Z from " << box.zMin()
     << " to " << box.zMax();
  return os;
}
}
}
