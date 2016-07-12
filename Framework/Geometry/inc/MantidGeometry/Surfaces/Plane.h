#ifndef PLANE_H
#define PLANE_H

#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidKernel/V3D.h"
#include <string>

namespace Mantid {

namespace Kernel {
template <typename T> class Matrix;
}

namespace Geometry {
//---------------------------------------------
// Forward declaration
//---------------------------------------------
class BaseVisit;

/**
\class Plane
\brief Holds a simple Plane
\author S. Ansell
\date April 2004
\version 1.0

Defines a plane and a normal and a distance from
the origin

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
*/
class MANTID_GEOMETRY_DLL Plane : public Quadratic {
private:
  Kernel::V3D NormV; ///< Normal vector
  double Dist;       ///< Distance

  std::size_t planeType() const; ///< are we alined on an axis
  Plane *doClone() const override;

protected:
  Plane(const Plane &) = default;
  Plane &operator=(const Plane &) = default;

public:
  /// Effective typename
  std::string className() const override { return "Plane"; }

  Plane();
  std::unique_ptr<Plane> clone() const;

  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }

  int setPlane(const Kernel::V3D &, const Kernel::V3D &);
  //  int setPlane(const std::string&);
  int side(const Kernel::V3D &) const override;
  int onSurface(const Kernel::V3D &) const override;
  // stuff for finding intersections etc.
  double dotProd(const Plane &) const;        ///< returns normal dot product
  Kernel::V3D crossProd(const Plane &) const; ///< returns normal cross product
  double
  distance(const Kernel::V3D &) const override; ///< distance from a point

  double getDistance() const { return Dist; } ///< Distance from origin
  const Kernel::V3D &getNormal() const {
    return NormV;
  } ///< Normal to plane (+ve surface)

  void rotate(const Kernel::Matrix<double> &) override;
  void displace(const Kernel::V3D &) override;

  int setSurface(const std::string &) override;
  void print() const override;
  void write(std::ostream &) const override; ///< Write in MCNPX form

  void setBaseEqn() override; ///< set up to be eqn based

  int LineIntersectionWithPlane(Kernel::V3D startpt, Kernel::V3D endpt,
                                Kernel::V3D &output);
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) override;
#ifdef ENABLE_OPENCASCADE
  TopoDS_Shape createShape() override;
#endif
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
