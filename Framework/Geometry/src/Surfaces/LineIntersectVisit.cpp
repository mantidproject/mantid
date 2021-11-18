// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/General.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Strings.h"
#include <algorithm>

namespace Mantid::Geometry {

LineIntersectVisit::LineIntersectVisit(const Kernel::V3D &point, const Kernel::V3D &unitVector)
    : m_line(point, unitVector)
/**
  Constructor
*/
{
  m_intersectionPointsOut.reserve(2);
  m_distancesOut.reserve(2);
}

void LineIntersectVisit::Accept(const Surface &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  (void)Surf; // Avoid compiler warning
  throw std::runtime_error("LineIntersectVisit::Accept Surface");
}

void LineIntersectVisit::Accept(const Quadratic &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPointsOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Plane &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPointsOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Cone &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPointsOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Cylinder &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPointsOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Sphere &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPointsOut, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const General &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPointsOut, Surf);
  procTrack();
}

void LineIntersectVisit::procTrack()
/**
  Sorts the m_intersectionPointsOut and distances
  with a closes first order.
*/
{
  // Calculate the distances to the points
  m_distancesOut.resize(m_intersectionPointsOut.size());
  using std::placeholders::_1;
  std::transform(m_intersectionPointsOut.begin(), m_intersectionPointsOut.end(), m_distancesOut.begin(),
                 std::bind(&Kernel::V3D::distance, m_line.getOrigin(), _1));
}

/**
 * @brief Prune duplicated interception points in the point list
 *
 */
void LineIntersectVisit::sortAndRemoveDuplicates() {
  const auto &u_vec = m_line.getDirect();
  const auto &origin = m_line.getOrigin();
  // sort the points by its distance to the track origin
  std::sort(m_intersectionPointsOut.begin(), m_intersectionPointsOut.end(),
            [*this, &u_vec, &origin](const Kernel::V3D &Pt_a, const Kernel::V3D &Pt_b) {
              const auto dist_a = u_vec.scalar_prod(Pt_a - origin);
              const auto dist_b = u_vec.scalar_prod(Pt_b - origin);
              return dist_a < dist_b;
            });
  // remove consecutive duplicated points
  auto last = std::unique(m_intersectionPointsOut.begin(), m_intersectionPointsOut.end(),
                          [*this](const Kernel::V3D &Pt_a, const Kernel::V3D &Pt_b) { return Pt_a == Pt_b; });
  // erase the tail
  m_intersectionPointsOut.erase(last, m_intersectionPointsOut.end());

  // update the distance list
  m_distancesOut.resize(m_intersectionPointsOut.size());
  std::transform(m_intersectionPointsOut.begin(), m_intersectionPointsOut.end(), m_distancesOut.begin(),
                 [*this, &u_vec, &origin](const Kernel::V3D &Pt) { return u_vec.scalar_prod(Pt - origin); });
}

} // namespace Mantid::Geometry
