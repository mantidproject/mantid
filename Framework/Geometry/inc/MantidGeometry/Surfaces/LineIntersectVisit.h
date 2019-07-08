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
  Line ATrack;                    ///< The line
  std::vector<Kernel::V3D> PtOut; ///< The intersection point
  std::vector<double> DOut;       ///< The distance

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
  const std::vector<double> &getDistance() const { return DOut; }
  /// Get the intersection points
  const std::vector<Kernel::V3D> &getPoints() const { return PtOut; }
  /// Get the number of intersection points
  unsigned long getNPoints() const { return (unsigned long)PtOut.size(); }

  /// Re-set the line
  void setLine(const Kernel::V3D &, const Kernel::V3D &);
};

} // namespace Geometry

} // NAMESPACE Mantid

#endif
