// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_LINEINTERSECTVISIT_H
#define MANTID_GEOMETRY_LINEINTERSECTVISIT_H

#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidKernel/V3D.h"
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
private:
  Line m_line;                                 ///< The line
  std::list<Kernel::V3D> m_intersectionPoints; ///< The intersection point
  std::list<double> m_distances;               ///< The distance

  void procTrack();

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
  const std::list<double> &getDistances() const { return m_distances; }
  /// Get the intersection points
  const std::list<Kernel::V3D> &getPoints() const {
    return m_intersectionPoints;
  }
  /// Get the number of intersection points
  size_t size() const { return m_intersectionPoints.size(); }

  /// Re-set the line
  void setLine(const Kernel::V3D &, const Kernel::V3D &);
};

} // namespace Geometry

} // NAMESPACE Mantid

#endif
