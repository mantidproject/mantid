// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/V3D.h"

#include <algorithm>
#include <cmath>

namespace Mantid {
namespace Geometry {
using Kernel::Tolerance;
using Kernel::V3D;

/**
 * Default constructor
 */
Track::Track() : m_startPoint(), m_unitVector(0., 0., 1.) {}

/**
 * Constructor
 * @param startPt :: Initial point
 * @param unitVector :: Directional vector. It must be unit vector.
 */
Track::Track(const V3D &startPt, const V3D &unitVector)
    : m_startPoint(startPt), m_unitVector(unitVector) {
  if (!unitVector.unitVector()) {
    throw std::invalid_argument(
        "Failed to construct track: direction is not a unit vector.");
  }
}

/**
 * Resets the track starting point and direction.
 * @param startPoint :: The new starting point
 * @param direction :: The new direction. Must be a unit vector!
 */
void Track::reset(const V3D &startPoint, const V3D &direction) {
  if (!direction.unitVector()) {
    throw std::invalid_argument(
        "Failed to reset track: direction is not a unit vector.");
  }
  m_startPoint = startPoint;
  m_unitVector = direction;
}

/**
 * Clear the current set of intersection results
 */
void Track::clearIntersectionResults() {
  m_links.clear();
  m_surfPoints.clear();
}

/**
 * Determines if the track is complete
 * @retval 0 :: Complete from Init to end without gaps
 * @retval +ve :: Index number of incomplete segment +1
 */
int Track::nonComplete() const {
  if (m_links.size() < 2) {
    return 0;
  }
  auto ac = m_links.cbegin();
  if (m_startPoint.distance(ac->entryPoint) > Tolerance) {
    return 1;
  }
  auto bc = ac;
  ++bc;

  while (bc != m_links.end()) {
    if ((ac->exitPoint).distance(bc->entryPoint) > Tolerance) {
      return (static_cast<int>(distance(m_links.begin(), bc)) + 1);
    }
    ++ac;
    ++bc;
  }
  // success
  return 0;
}

/**
 * Remove touching links that have identical
 * components
 */
void Track::removeCojoins() {
  if (m_links.empty()) {
    return;
  }
  auto prevNode = m_links.begin();
  auto nextNode = m_links.begin();
  ++nextNode;
  while (nextNode != m_links.end()) {
    if (prevNode->componentID == nextNode->componentID) {
      prevNode->exitPoint = nextNode->exitPoint;
      prevNode->distFromStart =
          prevNode->entryPoint.distance(prevNode->exitPoint);
      prevNode->distInsideObject = nextNode->distInsideObject;
      m_links.erase(nextNode);
      nextNode = prevNode;
      ++nextNode;
    } else {
      ++prevNode;
      ++nextNode;
    }
  }
}

/**
 * Objective is to merge in partial information about the beginning and end of
 * the tracks.
 * The points are kept in order
 * @param direction :: A flag indicating if the direction of travel is
 * entering/leaving
 * an object. +1 is entering, -1 is leaving.
 * @param endPoint :: Point of intersection
 * @param obj :: A reference to the object that was intersected
 * @param compID :: ID of the component that this link is about (Default=NULL)
 */
void Track::addPoint(const TrackDirection direction, const V3D &endPoint,
                     const IObject &obj, const ComponentID compID) {
  IntersectionPoint newPoint(direction, endPoint,
                             endPoint.distance(m_startPoint), obj, compID);
  auto lowestPtr =
      std::lower_bound(m_surfPoints.begin(), m_surfPoints.end(), newPoint);
  m_surfPoints.insert(lowestPtr, newPoint);
}

/**
 * This adds a whole segment to the track : This currently assumes that links
 * are added in order
 * @param firstPoint :: first Point
 * @param secondPoint :: second Point
 * @param distanceAlongTrack :: Distance along track
 * @param obj :: A reference to the object that was intersected
 * @param compID :: ID of the component that this link is about (Default=NULL)
 * @retval Index of link within the track
 */
int Track::addLink(const V3D &firstPoint, const V3D &secondPoint,
                   const double distanceAlongTrack, const IObject &obj,
                   const ComponentID compID) {
  // Process First Point
  Link newLink(firstPoint, secondPoint, distanceAlongTrack, obj, compID);
  int index(0);
  if (m_links.empty()) {
    m_links.push_back(newLink);
    index = 0;
  } else {
    auto linkPtr = std::lower_bound(m_links.begin(), m_links.end(), newLink);
    // must extract the distance before you insert otherwise the iterators are
    // incompatible
    index = static_cast<int>(std::distance(m_links.begin(), linkPtr));
    m_links.insert(linkPtr, newLink);
  }
  return index;
}

/**
 * Builds a set of linking track components.
 * This version deals with touching surfaces
 */
void Track::buildLink() {
  if (m_surfPoints.empty()) {
    return;
  }

  // The surface points were added in order when they were built so no sorting
  // is required here.
  auto ac = m_surfPoints.cbegin();
  auto bc = ac;
  ++bc;
  // First point is not necessarily in an object
  // Process first point:
  while (ac != m_surfPoints.end() &&
         ac->direction != TrackDirection::ENTERING) // stepping from an object.
  {
    if (ac->direction == TrackDirection::LEAVING) {
      addLink(m_startPoint, ac->endPoint, ac->distFromStart, *ac->object,
              ac->componentID); // from the void
    }
    ++ac;
    if (bc != m_surfPoints.end()) {
      ++bc;
    }
  }

  // have we now passed over all of the potential intersections without actually
  // hitting the object
  if (ac == m_surfPoints.end()) {
    // yes
    m_surfPoints.clear();
    return;
  }

  V3D workPt = ac->endPoint;       // last good point
  while (bc != m_surfPoints.end()) // Since bc > ac
  {
    if (ac->direction == TrackDirection::ENTERING &&
        bc->direction == TrackDirection::LEAVING) {
      // Touching surface / identical surface
      if (fabs(ac->distFromStart - bc->distFromStart) > Tolerance) {
        // track leave ac into bc.
        addLink(ac->endPoint, bc->endPoint, bc->distFromStart, *ac->object,
                ac->componentID);
      }
      // Points with intermediate void
      else {
        addLink(workPt, ac->endPoint, ac->distFromStart, *ac->object,
                ac->componentID);
      }
      workPt = bc->endPoint;

      // incrementing ac twice: since processing pairs
      ++ac;
      ++ac;
      ++bc; // can I do this past the end ?
      if (bc != m_surfPoints.end()) {
        ++bc;
      }
    } else // Test for glancing point / or void edges
    {      // These all can be skipped
      ++ac;
      ++bc;
    }
  }

  m_surfPoints.clear();
}

double Track::distInsideObject() const {
  return std::accumulate(m_links.begin(), m_links.end(), 0.,
                         [](double total, const auto &link) {
                           return total + link.distInsideObject;
                         });
}

std::ostream &operator<<(std::ostream &os, const Link &link) {
  os << link.entryPoint << "->" << link.exitPoint;
  return os;
}

std::ostream &operator<<(std::ostream &os, const TrackDirection &direction) {
  switch (direction) {
  case TrackDirection::ENTERING:
    os << "ENTERING";
    break;
  case TrackDirection::LEAVING:
    os << "LEAVING";
    break;
  case TrackDirection::INVALID:
    os << "INVALID";
    break;
  default:
    os.setstate(std::ios_base::failbit);
  }
  return os;
}

} // NAMESPACE Geometry

} // NAMESPACE Mantid
