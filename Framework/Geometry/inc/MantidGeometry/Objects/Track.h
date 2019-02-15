// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_TRACK_H_
#define MANTID_GEOMETRY_TRACK_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Tolerance.h"
#include <iosfwd>
#include <list>

namespace Mantid {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace Kernel {
class Logger;
}

namespace Geometry {

/**
\struct Link
\author S. Ansell
\author M. Gigg, Tessella plc
\brief For a leg of a track
*/
struct MANTID_GEOMETRY_DLL Link {
  /**
   * Constuctor
   * @param entry :: Kernel::V3D point to start
   * @param exit :: Kernel::V3D point to end track
   * @param totalDistance :: Total distance from start of track
   * @param obj :: A reference to the object that was intersected
   * @param compID :: An optional component identifier for the physical object
   * hit. (Default=NULL)
   */
  inline Link(const Kernel::V3D &entry, const Kernel::V3D &exit,
              const double totalDistance, const IObject &obj,
              const ComponentID compID = nullptr)
      : entryPoint(entry), exitPoint(exit), distFromStart(totalDistance),
        distInsideObject(entryPoint.distance(exitPoint)), object(&obj),
        componentID(compID) {}
  /// Less than operator
  inline bool operator<(const Link &other) const {
    return distFromStart < other.distFromStart;
  }
  /// Less than operator
  inline bool operator<(const double &other) const {
    return distFromStart < other;
  }

  /** @name Attributes. */
  //@{
  Kernel::V3D entryPoint;  ///< Entry point
  Kernel::V3D exitPoint;   ///< Exit point
  double distFromStart;    ///< Total distance from track beginning
  double distInsideObject; ///< Total distance covered inside object
  const IObject *object;   ///< The object that was intersected
  ComponentID componentID; ///< ComponentID of the intersected component
                           //@}
};

// calculations should prefer throwing a std::domain_error rather than using
// INVALID
enum class TrackDirection { LEAVING = -1, INVALID = 0, ENTERING = 1 };

inline bool operator<(const TrackDirection left, const TrackDirection right) {
  return static_cast<int>(left) < static_cast<int>(right);
}

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &os,
                                             TrackDirection direction);

/**
 * Stores a point of intersection along a track. The component intersected
 * is linked using its ComponentID.
 *
 * Ordering for IntersectionPoint is special since we need that when dist is
 * close that the +/- flag is taken into account.
 */
struct IntersectionPoint {
  /**
   * Constuctor
   * @param direction :: Indicates the direction of travel of the track with
   * respect to the object: +1 is entering, -1 is leaving.
   * @param end :: The end point for this partial segment
   * @param distFromStartOfTrack :: Total distance from start of track
   * @param compID :: An optional unique ID marking the component intersected.
   * (Default=NULL)
   * @param obj :: A reference to the object that was intersected
   */
  inline IntersectionPoint(const TrackDirection direction,
                           const Kernel::V3D &end,
                           const double distFromStartOfTrack,
                           const IObject &obj,
                           const ComponentID compID = nullptr)
      : direction(direction), endPoint(end),
        distFromStart(distFromStartOfTrack), object(&obj), componentID(compID) {
  }

  /**
   * A IntersectionPoint is less-than another if either
   * (a) the difference in distances is greater than the tolerance and this
   *distance is less than the other or
   * (b) the distance is less than the other and this point is defined as an
   *exit point
   *
   * @param other :: IntersectionPoint object to compare
   * @return True if the object is considered less than, otherwise false.
   */
  inline bool operator<(const IntersectionPoint &other) const {
    const double diff = fabs(distFromStart - other.distFromStart);
    return (diff > Kernel::Tolerance) ? distFromStart < other.distFromStart
                                      : direction < other.direction;
  }

  /** @name Attributes. */
  //@{
  TrackDirection direction; ///< Directional flag
  Kernel::V3D endPoint;     ///< Point
  double distFromStart;     ///< Total distance from track begin
  const IObject *object;    ///< The object that was intersected
  ComponentID componentID;  ///< Unique component ID
                            //@}
};

/**
 * Defines a track as a start point and a direction. Intersections are
 * stored as ordered lists of links from the start point to the exit point.
 *
 * @author S. Ansell
 */
class MANTID_GEOMETRY_DLL Track {
public:
  using LType = std::list<Link>;
  using PType = std::list<IntersectionPoint>;

public:
  /// Default constructor
  Track();
  /// Constructor
  Track(const Kernel::V3D &startPt, const Kernel::V3D &unitVector);
  /// Adds a point of intersection to the track
  void addPoint(const TrackDirection direction, const Kernel::V3D &endPoint,
                const IObject &obj, const ComponentID compID = nullptr);
  /// Adds a link to the track
  int addLink(const Kernel::V3D &firstPoint, const Kernel::V3D &secondPoint,
              const double distanceAlongTrack, const IObject &obj,
              const ComponentID compID = nullptr);
  /// Remove touching Links that have identical components
  void removeCojoins();
  /// Construct links between added points
  void buildLink();

  /// Set a starting point and direction
  void reset(const Kernel::V3D &startPoint, const Kernel::V3D &direction);
  /// Clear the current set of intersection results
  void clearIntersectionResults();
  /// Returns the starting point
  const Kernel::V3D &startPoint() const { return m_startPoint; }
  /// Returns the direction as a unit vector
  const Kernel::V3D &direction() const { return m_unitVector; }
  /// Returns an interator to the start of the set of links
  LType::iterator begin() { return m_links.begin(); }
  /// Returns an interator to one-past-the-end of the set of links
  LType::iterator end() { return m_links.end(); }
  /// Returns an interator to the start of the set of links (const version)
  LType::const_iterator begin() const { return m_links.begin(); }
  /// Returns an interator to one-past-the-end of the set of links (const
  /// version)
  LType::const_iterator end() const { return m_links.end(); }
  /// Returns an interator to the start of the set of links (const version)
  LType::const_iterator cbegin() const { return m_links.cbegin(); }
  /// Returns an interator to one-past-the-end of the set of links (const
  /// version)
  LType::const_iterator cend() const { return m_links.cend(); }
  /// Returns a reference to the first link
  LType::reference front() { return m_links.front(); }
  /// Returns a reference to the last link
  LType::reference back() { return m_links.back(); }
  /// Returns a reference to the first link (const version)
  LType::const_reference front() const { return m_links.front(); }
  /// Returns a reference to the last link (const version)
  LType::const_reference back() const { return m_links.back(); }
  /// Returns the number of links
  int count() const { return static_cast<int>(m_links.size()); }
  /// Is the link complete?
  int nonComplete() const;

private:
  Kernel::V3D m_startPoint; ///< Start Point
  Kernel::V3D m_unitVector; ///< unit vector to direction
  LType m_links;            ///< Track units
  PType m_surfPoints;       ///< Intersection points
};

} // NAMESPACE Geometry
} // NAMESPACE Mantid

#endif /*MANTID_GEOMETRY_TRACK_H_*/
