// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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
#include <boost/bind.hpp>

namespace Mantid {

namespace Geometry {

LineIntersectVisit::LineIntersectVisit(const Kernel::V3D &Pt,
                                       const Kernel::V3D &uVec)
    : m_line(Pt, uVec)
/**
  Constructor
*/
{}

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
  m_line.intersect(m_intersectionPoints, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Plane &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPoints, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Cone &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPoints, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Cylinder &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPoints, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const Sphere &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPoints, Surf);
  procTrack();
}

void LineIntersectVisit::Accept(const General &Surf)
/**
  Process an intersect track
  @param Surf :: Surface to use int line Interesect
*/
{
  m_line.intersect(m_intersectionPoints, Surf);
  procTrack();
}

void LineIntersectVisit::procTrack()
/**
  Sorts the PtOut and distances
  with a closes first order.
*/
{
  // Calculate the distances to the points
  m_distances.resize(m_intersectionPoints.size());
  std::transform(m_intersectionPoints.begin(), m_intersectionPoints.end(),
                 m_distances.begin(),
                 boost::bind(&Kernel::V3D::distance, m_line.getOrigin(), _1));
}

} // namespace Geometry

} // NAMESPACE Mantid
