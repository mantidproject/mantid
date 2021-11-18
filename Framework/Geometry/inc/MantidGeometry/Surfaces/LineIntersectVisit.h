// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidKernel/V3D.h"
#include <boost/container/small_vector.hpp>
#include <list>

namespace Mantid {

namespace Geometry {

//---------------------------------------------------------
// Forward declarations
//---------------------------------------------------------
class Surface;
class Quadratic;
class Plane;
class Sphere;
class Cone;
class Cylinder;
class General;

/**
\class LineIntersectVisit
\author S. Ansell
\version 1.0
\date September 2007
\brief Interset of Line with a surface

Creates interaction with a line
*/
class MANTID_GEOMETRY_DLL LineIntersectVisit : public BaseVisit {
public:
  using DistancesType = boost::container::small_vector<double, 5>;

private:
  Line m_line;                         ///< The line
  Line::PType m_intersectionPointsOut; ///< The intersection point
  DistancesType m_distancesOut;        ///< The distance

public:
  LineIntersectVisit(const Kernel::V3D &, const Kernel::V3D &);
  void Accept(const Surface &) override;
  void Accept(const Quadratic &);
  void Accept(const Plane &) override;
  void Accept(const Sphere &) override;
  void Accept(const Cone &) override;
  void Accept(const Cylinder &) override;
  void Accept(const General &) override;

  // Accessor
  /// Get the distance
  const DistancesType &getDistance() const { return m_distancesOut; }
  /// Get the intersection points
  const Line::PType &getPoints() const { return m_intersectionPointsOut; }
  /// Get the number of intersection points
  unsigned long getNPoints() const { return (unsigned long)m_intersectionPointsOut.size(); }

  /// Re-set the line
  void setLine(const Kernel::V3D &, const Kernel::V3D &);

  /// Prune out duplicated points and sort by distance to starting point
  void sortAndRemoveDuplicates();
};

} // namespace Geometry

} // NAMESPACE Mantid
